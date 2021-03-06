//##########################################################################
//#                                                                        #
//#                       CLOUDCOMPARE PLUGIN: qHPR                        #
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
//#               COPYRIGHT: Daniel Girardeau-Montaut                      #
//#                                                                        #
//##########################################################################

#ifndef Q_HPR_PLUGIN_HEADER
#define Q_HPR_PLUGIN_HEADER

//Qt
#include <QObject>

//CCLib
#include <ReferenceCloud.h>

#include "../ccStdPluginInterface.h"

class QAction;

//! Wrapper to the "Hidden Point Removal" algorithm for approximating points visibility in an N dimensional point cloud, as seen from a given viewpoint
/** "Direct Visibility of Point Sets", Sagi Katz, Ayellet Tal, and Ronen Basri. 
	SIGGRAPH 2007
	http://www.mathworks.com/matlabcentral/fileexchange/16581-hidden-point-removal
**/
class qHPR : public QObject, public ccStdPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(ccStdPluginInterface)

public:

	//! Default constructor
	qHPR(QObject* parent = 0);

    //inherited from ccPluginInterface
	virtual QString getName() const { return "Hidden Point Removal"; }
	virtual QString getDescription() const { return "Hidden Point Removal (Katz et al.)"; }
	virtual QIcon getIcon() const;

    //inherited from ccStdPluginInterface
	virtual void onNewSelection(const ccHObject::Container& selectedEntities);
    virtual void getActions(QActionGroup& group);

protected slots:

	//! Slot called when associated ation is triggered
	void doAction();

protected:

	//! Katz et al. algorithm
	CCLib::ReferenceCloud* removeHiddenPoints(CCLib::GenericIndexedCloudPersist* theCloud, const CCVector3& viewPoint, float fParam);

	//! Associated action
	QAction* m_action;
};

#endif
