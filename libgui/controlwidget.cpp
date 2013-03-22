/***************************************************************************
*               Generated by StarUML(tm) C++ Add-In                        *
***************************************************************************/
/***************************************************************************
*                                                                          *
*           Institute of Industrial and Control Engineering                *
*                 Technical University of Catalunya                        *
*                        Barcelona, Spain                                  *
*                                                                          *
*                Project Name:       Kautham Planner                       *
*                                                                          *
*     Copyright (C) 2007 - 2009 by Alexander Pérez and Jan Rosell          *
*            alexander.perez@upc.edu and jan.rosell@upc.edu                *
*                                                                          *
*             This is a motion planning tool to be used into               *
*             academic environment and it's provided without               *
*                     any warranty by the authors.                         *
*                                                                          *
*          Alexander Pérez is also with the Escuela Colombiana             *
*          de Ingeniería "Julio Garavito" placed in Bogotá D.C.            *
*             Colombia.  alexander.perez@escuelaing.edu.co                 *
*                                                                          *
***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
 
 
 
#include <QtGui>
#include "controlwidget.h"
#include <QString>


namespace libGUI {
	ControlWidget::ControlWidget( Robot* rob, Problem* prob, int offset) {
    _robot = rob;
    _globalOffset = offset;
    _ptProblem = prob;
    string names = "This|is|a|test";
    if(rob != NULL) names= rob->getControlsName();
		gridLayout = new QGridLayout(this);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    vboxLayout = new QVBoxLayout();
    vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
    QLabel* tempLab;
    QSlider* tempSli;
    QString content(names.c_str());
		QStringList cont = content.split("|");
    QStringList::const_iterator iterator;
    //int count=0;
    for (iterator = cont.constBegin(); iterator != cont.constEnd();
                    ++iterator)
    {
        //count++;
        //if(_globalOffset!=0 && count<_robot->getNumCoupledControls()) continue;

        tempLab = new QLabel(this);
        tempLab->setObjectName(/*QString("lbl")  +*/ (*iterator).toUtf8().constData());
        content = (*iterator).toUtf8().constData();
        tempLab->setText(content.append(" = 0.5"));
        this->vboxLayout->addWidget(tempLab);
        labels.push_back(tempLab);

        tempSli = new QSlider(this);
        tempSli->setObjectName(/*"sld" + */(*iterator).toUtf8().constData());
        tempSli->setOrientation(Qt::Horizontal);
        tempSli->setMinimum(0);
        tempSli->setMaximum(1000);
        tempSli->setSingleStep(1);
        tempSli->setValue(500);
        vboxLayout->addWidget(tempSli);
        sliders.push_back(tempSli);
        QObject::connect(tempSli,SIGNAL(valueChanged(int)),SLOT(sliderChanged(int)));
    }
	
    vboxLayout1 = new QVBoxLayout();
    btnUpdate = new QPushButton(this);
    btnUpdate->setText("Update Controls To Last Moved Sample");
    btnUpdate->setObjectName(QString::fromUtf8("Update Controls"));
    connect(btnUpdate, SIGNAL( clicked() ), this, SLOT( updateControls() ) ); 
    vboxLayout1->addWidget(btnUpdate);

    //values.resize(cont.size());
    values.resize(sliders.size());
    for(int i=0; i<values.size(); i++)
      values[i]=0.5;

    vboxLayout->addLayout(vboxLayout1);
    gridLayout->addLayout(vboxLayout,0,1,1,1);
		
	}

  ControlWidget::~ControlWidget(){
    for(unsigned int i=0; i<sliders.size(); i++){
      delete (QSlider*)sliders[i];
      delete (QLabel*)labels[i];
    }
    values.clear();
  }
  
	void ControlWidget::updateControls(){
        Sample *s  = _ptProblem->wSpace()->getLastSampleMovedTo();
        if(s!=NULL){
           int j=0;
           //the first controls are coupled with all the robots, then the values are
           //stored from the coordinates of the sample corresponding to the first robot
           for(; j < _robot->getNumCoupledControls(); j++ )
               values[j] = s->getCoords()[j];

           //the other values of the controls are read form the correspodning place in the sample vector
           //i.e. staring at globalOffset.
           for(; j < _robot->getNumControls(); j++ )
                values[j] = s->getCoords()[_globalOffset + j];

            setValues();
		}
	}

	void ControlWidget::sliderChanged(int value){
        QString tmp;
        for(unsigned int i=0; i<sliders.size(); i++){
            values[i]=(KthReal)((QSlider*)sliders[i])->value()/1000.0;

            tmp = labels[i]->text().left(labels[i]->text().indexOf("=") + 2);
            labels[i]->setText( tmp.append( QString().setNum(values[i],'g',5)));
        }
        _ptProblem->setCurrentControls(values,_globalOffset);
        //if(_robot != NULL) _robot->control2Pose(values);

        //move the robots
        Sample *s=_ptProblem->wSpace()->getLastSampleMovedTo();

        vector<KthReal> coords;
        if(s==NULL){
            coords.resize(_ptProblem->wSpace()->getDimension());
            for(int i=0; i < _ptProblem->wSpace()->getDimension(); i++ ) coords[i] = 0.5;
        }
        else{
            //vector<KthReal>& coords = s->getCoords();
            coords = s->getCoords();

        }


        int j=0;
        for(; j < _robot->getNumCoupledControls(); j++ ){
            coords[j]=values[j];
            coords[_globalOffset+j]=values[j];//dummy, not used in moveRobotsTo
        }
        for(; j < _robot->getNumControls(); j++ )
            coords[_globalOffset+j]=values[j];

        //Sample *s2 = new Sample(s);
        Sample *s2 = new Sample(_ptProblem->wSpace()->getDimension());

        s2->setCoords(coords);
        _ptProblem->wSpace()->moveRobotsTo(s2);
    }

  void ControlWidget::setValues(){
      vector<KthReal>   values2;
      values2.resize(values.size());
      for(unsigned int i = 0; i < values.size(); i++)
            values2[i] = values[i];

      for(unsigned int i = 0; i < values.size(); i++)
        ((QSlider*)sliders[i])->setValue((int)(values2[i]*1000.0));
    }

}

