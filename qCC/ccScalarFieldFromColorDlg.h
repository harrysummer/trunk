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

#ifndef CC_SF_FROM_COLOR_DLG_HEADER
#define CC_SF_FROM_COLOR_DLG_HEADER

#include <ui_scalarFieldFromColorDlg.h>

class ccPointCloud;

//! Dialog to choose 2 scalar fields (SF) and one operation for arithmetics processing
class ccScalarFieldFromColorDlg : public QDialog, public Ui::scalarFieldFromColorDlg
{
public:

    //! Default constructor
    ccScalarFieldFromColorDlg(QWidget* parent = 0);

    //! Returns if to export R channel as SF
    bool getRStatus();

    //! Returns if to export G channel as SF
    bool getGStatus();

    //! Returns if to export B channel as SF
    bool getBStatus();

    //! Returns if to export Composite channel as SF
    bool getCompositeStatus();


};

#endif
