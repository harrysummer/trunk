//##########################################################################
//#                                                                        #
//#                            CLOUDCOMPARE                                #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#          COPYRIGHT: EDF R&D / TELECOM ParisTech (ENST-TSI)             #
//#                                                                        #
//##########################################################################

#include "OFFFilter.h"
#include "ccCoordinatesShiftManager.h"

//Qt
#include <QApplication>
#include <QFile>
#include <QTextStream>

#include <QFileInfo>
#include <QStringList>
#include <QString>
#include <QMessageBox>
#include <QPushButton>
#include <QProgressDialog>

//qCC_db
#include <ccLog.h>
#include <ccMesh.h>
#include <ccPointCloud.h>
#include <ccProgressDialog.h>
#include <ccNormalVectors.h>
#include <ccOctree.h>

//System
#include <string.h>

CC_FILE_ERROR OFFFilter::saveToFile(ccHObject* entity, const char* filename)
{
	if (!entity)
		return CC_FERR_BAD_ARGUMENT;

	if (!entity->isKindOf(CC_TYPES::MESH))
		return CC_FERR_BAD_ENTITY_TYPE;

	ccGenericMesh* mesh = ccHObjectCaster::ToGenericMesh(entity);
	if (mesh->size() == 0)
	{
		ccLog::Warning(QString("[OFF] No facet in mesh '%1'!").arg(mesh->getName()));
		return CC_FERR_NO_ERROR;
	}
	ccGenericPointCloud* vertices = mesh->getAssociatedCloud();
	if (!vertices || vertices->size() == 0)
	{
		ccLog::Warning(QString("[OFF] No vertices in mesh '%1'?!").arg(mesh->getName()));
		return CC_FERR_NO_ERROR;
	}

	//try to open file for saving
	QFile fp(filename);
	if (!fp.open(QIODevice::WriteOnly | QIODevice::Text))
		return CC_FERR_WRITING;

	QTextStream stream(&fp);
	stream.setRealNumberPrecision(12); //TODO: ask the user?
    
    //header: "OFF"
    stream << "OFF" << endl;
    
    //2nd line: vertices count / faces count / edges count
    unsigned vertCount = vertices->size();
    unsigned triCount = mesh->size();
    stream << vertCount << ' ' << triCount << ' ' << 0 << endl;
    
    //save vertices
	{
		for (unsigned i=0; i<vertCount; ++i)
		{
			const CCVector3* P = vertices->getPoint(i);
			CCVector3d Pglobal = vertices->toGlobal3d<PointCoordinateType>(*P);
			stream << Pglobal.x << ' ' << Pglobal.y << ' ' << Pglobal.z << endl;
		}
	}
    
    //save triangles
	{
		for (unsigned i=0; i<triCount; ++i)
		{
			const CCLib::TriangleSummitsIndexes* tsi = mesh->getTriangleIndexes(i);
			stream << "3 " << tsi->i1 << ' ' << tsi->i2 << ' ' << tsi->i3 << endl;
		}
    }

	return CC_FERR_NO_ERROR;
}


static QString GetNextLine(QTextStream& stream)
{
	QString currentLine;
	//skip comments
	do
	{
		currentLine = stream.readLine();
		//end of file?
		if (currentLine.isNull())
			return QString();
	}
	while (currentLine.startsWith("#") || currentLine.isEmpty());

	return currentLine;
}

CC_FILE_ERROR OFFFilter::loadFile(const char* filename, ccHObject& container, bool alwaysDisplayLoadDialog/*=true*/, bool* coordinatesShiftEnabled/*=0*/, CCVector3d* coordinatesShift/*=0*/)
{
	//try to open file
	QFile fp(filename);
	if (!fp.open(QIODevice::ReadOnly | QIODevice::Text))
		return CC_FERR_READING;

	QTextStream stream(&fp);

	QString currentLine = stream.readLine();
	if (!currentLine.toUpper().startsWith("OFF"))
		return CC_FERR_MALFORMED_FILE;

	//check if the number of vertices/faces/etc. are on the first line (yes it happens :( )
	QStringList tokens = currentLine.split(QRegExp("\\s+"),QString::SkipEmptyParts);
	if (tokens.size() == 4)
	{
		tokens.removeAt(0);
	}
	else
	{
		currentLine = GetNextLine(stream);

		//end of file already?!
		if (currentLine.isNull())
			return CC_FERR_MALFORMED_FILE;

		while (currentLine.startsWith("#") || currentLine.isEmpty());

		//read the number of vertices/faces
		tokens = currentLine.split(QRegExp("\\s+"),QString::SkipEmptyParts);
		if (tokens.size() < 2/*3*/) //should be 3 but we only use the 2 firsts...
			return CC_FERR_MALFORMED_FILE;
	}

	bool ok = false;
	unsigned vertCount = tokens[0].toUInt(&ok);
	if (!ok)
		return CC_FERR_MALFORMED_FILE;
	unsigned triCount = tokens[1].toUInt(&ok);
	if (!ok)
		return CC_FERR_MALFORMED_FILE;

	//create cloud and reserve some memory
	ccPointCloud* vertices = new ccPointCloud("vertices");
	if (!vertices->reserve(vertCount))
	{
		delete vertices;
		return CC_FERR_NOT_ENOUGH_MEMORY;
	}

	//read vertices
	{
		CCVector3d Pshift(0,0,0);
		for (unsigned i=0; i<vertCount; ++i)
		{
			currentLine = GetNextLine(stream);
			tokens = currentLine.split(QRegExp("\\s+"),QString::SkipEmptyParts);
			if (tokens.size() < 3)
			{
				delete vertices;
				return CC_FERR_MALFORMED_FILE;
			}

			//read vertex
			double Pd[3];
			{
				bool vertexIsOk = false;
				Pd[0] = tokens[0].toDouble(&vertexIsOk);
				if (vertexIsOk)
				{
					Pd[1] = tokens[1].toDouble(&vertexIsOk);
					if (vertexIsOk)
						Pd[2] = tokens[2].toDouble(&vertexIsOk);
				}
				if (!vertexIsOk)
				{
					delete vertices;
					return CC_FERR_MALFORMED_FILE;
				}
			}

			//first point: check for 'big' coordinates
			if (i == 0)
			{
				bool shiftAlreadyEnabled = (coordinatesShiftEnabled && *coordinatesShiftEnabled && coordinatesShift);
				if (shiftAlreadyEnabled)
					Pshift = *coordinatesShift;
				bool applyAll = false;
				if (	sizeof(PointCoordinateType) < 8
					&&	ccCoordinatesShiftManager::Handle(Pd,0,alwaysDisplayLoadDialog,shiftAlreadyEnabled,Pshift,0,applyAll))
				{
					vertices->setGlobalShift(Pshift);
					ccLog::Warning("[OFF] Cloud has been recentered! Translation: (%.2f,%.2f,%.2f)",Pshift.x,Pshift.y,Pshift.z);

					//we save coordinates shift information
					if (applyAll && coordinatesShiftEnabled && coordinatesShift)
					{
						*coordinatesShiftEnabled = true;
						*coordinatesShift = Pshift;
					}
				}
			}

			CCVector3 P(static_cast<PointCoordinateType>(Pd[0] + Pshift.x),
						static_cast<PointCoordinateType>(Pd[1] + Pshift.y),
						static_cast<PointCoordinateType>(Pd[2] + Pshift.z));

			vertices->addPoint(P);
		}
	}

	ccMesh* mesh = new ccMesh(vertices);
	mesh->addChild(vertices);
	if (!mesh->reserve(triCount))
	{
		delete mesh;
		return CC_FERR_NOT_ENOUGH_MEMORY;
	}

	//load triangles
	{
		bool ignoredPolygons = false;
		for (unsigned i=0; i<triCount; ++i)
		{
			currentLine = GetNextLine(stream);
			tokens = currentLine.split(QRegExp("\\s+"),QString::SkipEmptyParts);
			if (tokens.size() < 3)
			{
				delete mesh;
				return CC_FERR_MALFORMED_FILE;
			}

			unsigned polyVertCount = tokens[0].toUInt(&ok);
			if (!ok || static_cast<int>(polyVertCount) >= tokens.size())
			{
				delete mesh;
				return CC_FERR_MALFORMED_FILE;
			}
			if (polyVertCount == 3 || polyVertCount == 4)
			{
				//decode indexes
				unsigned indexes[4];
				for (unsigned j=0; j<polyVertCount; ++j)
				{
					indexes[j] = tokens[j+1].toUInt(&ok);
					if (!ok)
					{
						delete mesh;
						return CC_FERR_MALFORMED_FILE;
					}
				}

				//reserve memory if necessary
				unsigned polyTriCount = polyVertCount-2;
				if (mesh->size() + polyTriCount > mesh->maxSize())
				{
					if (!mesh->reserve(mesh->size() + polyTriCount + 256)) //take some advance to avoid too many allocations
					{
						delete mesh;
						return CC_FERR_NOT_ENOUGH_MEMORY;
					}
				}

				//triangle or quad only
				mesh->addTriangle(indexes[0],indexes[1],indexes[2]);
				if (polyVertCount == 4)
					mesh->addTriangle(indexes[0],indexes[2],indexes[3]);

			}
			else
			{
				ignoredPolygons = true;
			}
		}

		if (ignoredPolygons)
		{
			ccLog::Warning("[OFF] Some polygons with an unhandled size (i.e. > 4) were ignored!");
		}
	}

	if (mesh->size() == 0)
	{
		ccLog::Warning("[OFF] Failed to load any polygon!");
		mesh->detachChild(vertices);
		delete mesh;
		mesh = 0;

		container.addChild(vertices);
		vertices->setEnabled(true);
	}
	else
	{
		if (mesh->size() < mesh->maxSize())
			mesh->resize(mesh->size());

		//DGM: normals can be per-vertex or per-triangle so it's better to let the user do it himself later
		//Moreover it's not always good idea if the user doesn't want normals (especially in ccViewer!)
		//if (mesh->computeNormals())
		//	mesh->showNormals(true);
		//else
		//	ccLog::Warning("[OFF] Failed to compute per-vertex normals...");
		ccLog::Warning("[OFF] Mesh has no normal! You can manually compute them (select it then call \"Edit > Normals > Compute\")");

		vertices->setEnabled(false);
		//vertices->setLocked(true); //DGM: no need to lock it as it is only used by one mesh!
		container.addChild(mesh);
	}

	return CC_FERR_NO_ERROR;
}
