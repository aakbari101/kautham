/*************************************************************************\
   Copyright 2014 Institute of Industrial and Control Engineering (IOC)
                 Universitat Politecnica de Catalunya
                 BarcelonaTech
    All Rights Reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 \*************************************************************************/

/* Author: Alexander Perez, Jan Rosell, Nestor Garcia Hidalgo */
 
 
#include <QtGui>
#include "controlwidget.h"
#include <QString>


namespace Kautham {

    ControlWidget::ControlWidget(Problem* prob, vector<DOFWidget*> DOFWidgets, bool robot) {
        _ptProblem = prob;
        robWidget = robot;
        _DOFWidgets = DOFWidgets;
        string names = "This|is|a|test";
        if (robot) {
            names = _ptProblem->wSpace()->getRobControlsName();
        } else {
            names = _ptProblem->wSpace()->getObsControlsName();
        }
        gridLayout = new QGridLayout(this);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        vboxLayout = new QVBoxLayout();
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        QLabel* tempLab;
        QSlider* tempSli;
        QString content(names.c_str());
        QStringList cont = content.split("|");
        QStringList::const_iterator iterator;
        for (iterator = cont.constBegin(); iterator != cont.constEnd();
             ++iterator) {
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
        if (robWidget) {
            btnUpdate->setText("Update Controls To Last Moved Sample");
        } else {
            btnUpdate->setText("Update Controls To Initial Sample");
        }
        btnUpdate->setObjectName(QString::fromUtf8("Update Controls"));
        connect(btnUpdate, SIGNAL( clicked() ), this, SLOT( updateControls() ) );
        vboxLayout1->addWidget(btnUpdate);

        values.resize(sliders.size());
        for(int i=0; i<values.size(); i++)
            values[i]=0.5;

        vboxLayout->addLayout(vboxLayout1);
        gridLayout->addLayout(vboxLayout,0,1,1,1);

    }

    ControlWidget::~ControlWidget(){
        for(uint i = 0; i < sliders.size(); i++) {
            delete (QSlider*)sliders[i];
            delete (QLabel*)labels[i];
        }
        sliders.clear();
        labels.clear();
        values.clear();
        for (uint i = 0; _DOFWidgets.size(); i++) {
            delete (DOFWidget*)_DOFWidgets[i];
        }
        _DOFWidgets.clear();
        delete gridLayout;
        delete vboxLayout;
        delete vboxLayout1;
        delete btnUpdate;
    }

    void ControlWidget::updateControls(){
        Sample *s;
        if (robWidget) {
            s  = _ptProblem->wSpace()->getLastRobSampleMovedTo();
        } else {
            s  = _ptProblem->wSpace()->getInitObsSample();
        }
        if (s != NULL){
            setValues(s->getCoords());
        }
    }

    void ControlWidget::sliderChanged(int value){
        QString tmp;
        for(unsigned int i=0; i<sliders.size(); i++){
            values[i]=(KthReal)((QSlider*)sliders[i])->value()/1000.0;

            tmp = labels[i]->text().left(labels[i]->text().indexOf("=") + 2);
            labels[i]->setText( tmp.append( QString().setNum(values[i],'g',5)));
        }

        if (robWidget) {
            _ptProblem->setCurrentRobControls(values);
        } else {
            _ptProblem->setCurrentObsControls(values);
        }

        Sample *sample = new Sample(values.size());
        sample->setCoords(values);

        vector <float> params;
        if (robWidget) {
            _ptProblem->wSpace()->moveRobotsTo(sample);
            for (uint i = 0; i < _DOFWidgets.size(); i++) {
                _ptProblem->wSpace()->getRobot(i)->control2Parameters(values,params);
                ((DOFWidget*)_DOFWidgets.at(i))->setValues(params);
            }
        } else {
            _ptProblem->wSpace()->moveObstaclesTo(sample);
            for (uint i = 0; i < _DOFWidgets.size(); i++) {
                _ptProblem->wSpace()->getObstacle(i)->control2Parameters(values,params);
                ((DOFWidget*)_DOFWidgets.at(i))->setValues(params);
            }
        }
    }

    void ControlWidget::setValues(vector<KthReal> coords){
        QString tmp;
        for(unsigned int i = 0; i < coords.size(); i++) {
            ((QSlider*)sliders[i])->setValue((int)(coords[i]*1000.0));

            tmp = labels[i]->text().left(labels[i]->text().indexOf("=") + 2);
            labels[i]->setText( tmp.append( QString().setNum(coords[i],'g',5)));
        }

        for (int j = 0; j < values.size(); j++)
            values[j] = coords[j];

        if (robWidget) {
            _ptProblem->setCurrentRobControls(values);
        } else {
            _ptProblem->setCurrentObsControls(values);
        }

        Sample *sample = new Sample(values.size());
        sample->setCoords(values);

        vector <float> params;
        if (robWidget) {
            _ptProblem->wSpace()->moveRobotsTo(sample);
            for (uint i = 0; i < _DOFWidgets.size(); i++) {
                _ptProblem->wSpace()->getRobot(i)->control2Parameters(values,params);
                ((DOFWidget*)_DOFWidgets.at(i))->setValues(params);
            }
        } else {
            _ptProblem->wSpace()->moveObstaclesTo(sample);
            for (uint i = 0; i < _DOFWidgets.size(); i++) {
                _ptProblem->wSpace()->getObstacle(i)->control2Parameters(values,params);
                ((DOFWidget*)_DOFWidgets.at(i))->setValues(params);
            }
        }
    }

}

