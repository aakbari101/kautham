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
 
#include "plannerwidget.h"
#include "gui.h"
#include <sampling/se3conf.h>
#include <sampling/robconf.h>

#if defined(KAUTHAM_USE_IOC)
#include <planner/ioc/iocplanner.h>
#endif

namespace Kautham {
    PlannerWidget::PlannerWidget(Planner* plan, SampleSet* samp, bool camera):KauthamWidget(plan){
        _samples = samp;
        _planner = plan;
        _stepSim = 0;
        _ismoving = false;

        QGroupBox *groupBox = new QGroupBox(this);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));

        QGridLayout *gridLayoutGB = new QGridLayout(groupBox);
        gridLayoutGB->setObjectName(QString::fromUtf8("gridLayout"));

        QVBoxLayout *verticalLayoutGB = new QVBoxLayout();
        verticalLayoutGB->setObjectName(QString::fromUtf8("verticalLayout"));

        QHBoxLayout *horizontalLayoutGB = new QHBoxLayout();
        horizontalLayoutGB->setObjectName(QString::fromUtf8("horizontalLayout"));

        QLabel *labelGB = new QLabel(groupBox);
        labelGB->setObjectName(QString::fromUtf8("label"));

        horizontalLayoutGB->addWidget(labelGB);

        localFromBox = new QSpinBox(groupBox);
        localFromBox->setObjectName(QString::fromUtf8("localFromBox"));

        horizontalLayoutGB->addWidget(localFromBox);

        QLabel *label_2GB = new QLabel(groupBox);
        label_2GB->setObjectName(QString::fromUtf8("label_2"));

        horizontalLayoutGB->addWidget(label_2GB);

        localToBox = new QSpinBox(groupBox);
        localToBox->setObjectName(QString::fromUtf8("localToBox"));

        horizontalLayoutGB->addWidget(localToBox);


        verticalLayoutGB->addLayout(horizontalLayoutGB);

        QHBoxLayout *horizontalLayout_2GB = new QHBoxLayout();
        horizontalLayout_2GB->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        _cmbTry = new QPushButton(groupBox);
        _cmbTry->setObjectName(QString::fromUtf8("_cmbTry"));

        horizontalLayout_2GB->addWidget(_cmbTry);

        connectLabel = new QLabel(groupBox);
        connectLabel->setObjectName(QString::fromUtf8("label_3"));
        connectLabel->setPixmap(QPixmap(QString::fromUtf8(":/icons/tryconnect.xpm")));

        horizontalLayout_2GB->addWidget(connectLabel);


        verticalLayoutGB->addLayout(horizontalLayout_2GB);


        gridLayoutGB->addLayout(verticalLayoutGB, 0, 0, 1, 1);


        vboxLayout->addWidget(groupBox);

        tmpLabel = new QLabel(this);
        tmpLabel->setText("Init configuration is the sample:");
        globalFromBox = new QSpinBox(this);

        hboxLayout = new QHBoxLayout();
        hboxLayout->addWidget(tmpLabel);
        hboxLayout->addWidget(globalFromBox);
        vboxLayout->addLayout(hboxLayout);

        tmpLabel = new QLabel(this);
        tmpLabel->setText("Goal configuration is the sample:");
        globalToBox = new QSpinBox(this);

        hboxLayout = new QHBoxLayout();
        hboxLayout->addWidget(tmpLabel);
        hboxLayout->addWidget(globalToBox);
        vboxLayout->addLayout(hboxLayout);

        if(camera = true){
            chkCamera = new QCheckBox("Move the camera.");
            chkCamera->setChecked(false);
            vboxLayout->addWidget(chkCamera);
        }

        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));

        btnGetPath = new QPushButton(this);
        btnGetPath->setObjectName(QString::fromUtf8("getPathButton"));

        hboxLayout->addWidget(btnGetPath);

        btnSaveData = new QPushButton(this);
        btnSaveData->setObjectName(QString::fromUtf8("saveButton"));
        btnSaveData->setDisabled(true);
        hboxLayout->addWidget(btnSaveData);

        vboxLayout->addLayout(hboxLayout);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));


        btnLoadData = new QPushButton(this);
        btnLoadData->setObjectName(QString::fromUtf8("loadButton"));

        hboxLayout2->addWidget(btnLoadData);

        moveButton = new QPushButton(this);
        moveButton->setObjectName(QString::fromUtf8("moveButton"));
        moveButton->setEnabled(false);

        hboxLayout2->addWidget(moveButton);

        vboxLayout->addLayout(hboxLayout2);

        btnGetPath->setText(QApplication::translate("Form", "Get Path", 0, QApplication::UnicodeUTF8));
        btnSaveData->setText(QApplication::translate("Form", "Save Data", 0, QApplication::UnicodeUTF8));
        btnLoadData->setText(QApplication::translate("Form", "Load Data", 0, QApplication::UnicodeUTF8));
        moveButton->setText(QApplication::translate("Form", "Start Move ", 0, QApplication::UnicodeUTF8));

        groupBox->setTitle(QApplication::translate("Form", "Local Planner", 0, QApplication::UnicodeUTF8));
        labelGB->setText(QApplication::translate("Form", "From:", 0, QApplication::UnicodeUTF8));
        label_2GB->setText(QApplication::translate("Form", "To:", 0, QApplication::UnicodeUTF8));
        _cmbTry->setText(QApplication::translate("Form", "Try Connect", 0, QApplication::UnicodeUTF8));
        connectLabel->setText(QString());

        _plannerTimer = new QTimer( this );

        if(_planner != NULL ){
            connect(btnGetPath, SIGNAL( clicked() ), this, SLOT( getPath() ) );
            connect(btnSaveData, SIGNAL( clicked() ), this, SLOT( saveData() ) );
            connect(btnLoadData, SIGNAL( clicked() ), this, SLOT( loadData() ) );
            connect(moveButton, SIGNAL( clicked() ), this, SLOT( simulatePath() ) );
            connect(_plannerTimer, SIGNAL(timeout()), this, SLOT(moveAlongPath()) );
            connect(globalFromBox, SIGNAL( valueChanged( int )), this, SLOT( showSample( int )));
            connect(globalToBox, SIGNAL( valueChanged( int )), this, SLOT( showSample( int )));
            connect(localFromBox, SIGNAL( valueChanged( int )), this, SLOT( showSample( int )));
            connect(localToBox, SIGNAL( valueChanged( int )), this, SLOT( showSample( int )));
            connect(_cmbTry, SIGNAL( clicked() ), this, SLOT( tryConnect( )));
            connect(chkCamera, SIGNAL( clicked() ), this, SLOT( chkCameraClick( )));

            globalFromBox->setValue( 0 );
            globalToBox->setValue( 1 );
            localFromBox->setValue( 0 );
            localToBox->setValue( 1 );

        }
    }


    PlannerWidget::~PlannerWidget() {
        if (_ismoving) {
            simulatePath();
        }
        delete hboxLayout;
        delete hboxLayout2;
        delete btnGetPath;
        delete btnSaveData;
        delete moveButton;
        delete chkCamera;
        delete btnLoadData;
        delete globalFromBox;
        delete globalToBox;
        delete tmpLabel;
        delete _plannerTimer;
        delete label;
        delete localFromBox;
        delete label_2;
        delete localToBox;
        delete horizontalLayout_2;
        delete _cmbTry;
        delete connectLabel;
    }


    void PlannerWidget::tryConnect() {
        if (_planner != NULL) {
            switch ((int)_planner->getFamily()) {
#if defined(KAUTHAM_USE_IOC)
            case IOCPLANNER:
                tryConnectIOC();
                break;
#endif
#if defined(KAUTHAM_USE_OMPL)
            case OMPLPLANNER:
                tryConnectOMPL();
                break;
            case OMPLCPLANNER:
                tryConnectOMPLC();
                break;
#if defined(KAUTHAM_USE_ODE)
            case ODEPLANNER:
                tryConnectODE();
                break;
#endif
#endif
            case NOFAMILY:
                writeGUI("The planner is not configured properly!!. Something is wrong with your application.");
                break;
            default:
                writeGUI("The planner is not configured properly!!. Something is wrong with your application.");
                break;
            }
        } else {
            writeGUI("The planner is not configured properly!!. Something is wrong with your application.");
        }
    }


    void PlannerWidget::tryConnectIOC() {
#if defined(KAUTHAM_USE_IOC)
        Sample *fromSample = _samples->getSampleAt(localFromBox->text().toInt());
        Sample *toSample = _samples->getSampleAt(localToBox->text().toInt());
        ((IOC::iocPlanner*)_planner)->getLocalPlanner()->setInitSamp(fromSample);
        ((IOC::iocPlanner*)_planner)->getLocalPlanner()->setGoalSamp(toSample);

        KthReal distance = ((IOC::iocPlanner*)_planner)->getLocalPlanner()->
                distance(fromSample,toSample);

        writeGUI("Distance:  "+QString::number(distance).toStdString());
        if (((IOC::iocPlanner*)_planner)->getLocalPlanner()->canConect()) {
            connectLabel->setPixmap(QPixmap(QString::fromUtf8(":/icons/connect.xpm")));
            writeGUI("The samples can be connected.");
        } else {
            connectLabel->setPixmap(QPixmap(QString::fromUtf8(":/icons/noconnect.xpm")));
            writeGUI("The samples can NOT be connected.");
        }
#endif
    }


    void PlannerWidget::tryConnectOMPL() {
#if defined(KAUTHAM_USE_OMPL)
        ob::CompoundState *fromState;
        ((omplplanner::omplPlanner*)_planner)->
                smp2omplState(_samples->getSampleAt(localFromBox->text().toInt()),fromState);

        ob::CompoundState *toState;
        ((omplplanner::omplPlanner*)_planner)->
                smp2omplState(_samples->getSampleAt(localToBox->text().toInt()),toState);

        bool connected = ((ob::MotionValidator *)((ob::SpaceInformation *)((omplplanner::omplPlanner*)
                                                                           _planner)->
                                                  SimpleSetup()->getSpaceInformation().get())->
                          getMotionValidator().get())->checkMotion(fromState,fromState);

        if (connected) {
            connectLabel->setPixmap(QPixmap(QString::fromUtf8(":/icons/connect.xpm")));
            writeGUI("The samples can be connected.");
        } else {
            connectLabel->setPixmap(QPixmap(QString::fromUtf8(":/icons/noconnect.xpm")));
            writeGUI("The samples can NOT be connected.");
        }
#endif
    }


    void PlannerWidget::tryConnectOMPLC() {
#if defined(KAUTHAM_USE_OMPL)
        writeGUI("Sorry: Nothing implemented yet for non-ioc planners");
#endif
    }


    void PlannerWidget::tryConnectODE() {
#if defined(KAUTHAM_USE_OMPL) && defined(KAUTHAM_USE_ODE)
        writeGUI("Sorry: Nothing implemented yet for non-ioc planners");
#endif
    }


    void PlannerWidget::getPath() {
        emit changeCursor(true);
        moveButton->setEnabled(false);
        if (_planner != NULL ) {
            _planner->wkSpace()->moveObstaclesTo(_planner->wkSpace()->getInitObsSample());
            _planner->setInitSamp(_samples->getSampleAt(globalFromBox->text().toInt()));
            _planner->setGoalSamp(_samples->getSampleAt(globalToBox->text().toInt()));
            bool result = _planner->solveAndInherit();
            moveButton->setEnabled(result);
            btnSaveData->setEnabled(result);
        }
        emit changeCursor(false);
    }


    void PlannerWidget::saveData(){
        if (_planner != NULL) {
            switch ((int)_planner->getFamily()) {
#if defined(KAUTHAM_USE_IOC)
            case IOCPLANNER:
                saveDataIOC();
                break;
#endif
#if defined(KAUTHAM_USE_OMPL)
            case OMPLPLANNER:
                saveDataOMPL();
                break;
            case OMPLCPLANNER:
                saveDataOMPLC();
                break;
#if defined(KAUTHAM_USE_ODE)
            case ODEPLANNER:
                saveDataODE();
                break;
#endif
#endif
            case NOFAMILY:
                writeGUI("The planner is not configured properly!!. Something is wrong with your application.");
                break;
            default:
                writeGUI("The planner is not configured properly!!. Something is wrong with your application.");
                break;
            }
        } else {
            writeGUI("The planner is not configured properly!!. Something is wrong with your application.");
        }
    }


    void PlannerWidget::saveDataIOC() {
#if defined(KAUTHAM_USE_IOC)
        emit changeCursor(true);
        QString filePath = getFilePath();
        if (!filePath.isEmpty()) {
            sendText(QString("Kautham is saving a planner data in a file: " + filePath).toUtf8().constData() );
            QString dir = filePath;
            dir.truncate(dir.lastIndexOf("/"));
            ((IOC::iocPlanner*)_planner)->saveData(filePath.toUtf8().constData());
        }
        setTable(_planner->getParametersAsString());
        emit changeCursor(false);
#endif
    }

    void PlannerWidget::saveDataOMPL() {
#if defined(KAUTHAM_USE_OMPL)
        emit changeCursor(true);
        QString filePath = getFilePath();
        if (!filePath.isEmpty()) {
            sendText(QString("Kautham is saving a planner data in a file: "+filePath).toUtf8().constData());
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                stringstream sstr;
                ((omplplanner::omplPlanner*)_planner)->SimpleSetup()->getSolutionPath().print(sstr);
                QTextStream out(&file);
                out << sstr.str().c_str();
                stringstream sstr2;
                ((omplplanner::omplPlanner*)_planner)->SimpleSetup()->getSolutionPath().printAsMatrix(sstr2);
                out << endl << sstr2.str().c_str();
                sendText("File was saved successfully");
            } else {
                sendText("Sorry but the file couldn't be saved");
            }
            file.close();
        }
        emit changeCursor(false);
#endif
    }


    void PlannerWidget::saveDataOMPLC() {
#if defined(KAUTHAM_USE_OMPL)
        writeGUI("Sorry: Nothing implemented yet for non-ioc planners");
#endif
    }


    void PlannerWidget::saveDataODE() {
#if defined(KAUTHAM_USE_OMPL) && defined(KAUTHAM_USE_ODE)
        writeGUI("Sorry: Nothing implemented yet for non-ioc planners");
#endif
    }


    QString PlannerWidget::getFilePath() {
        QSettings settings("IOC","Kautham");
        QDir workDir;
        QString last_path = settings.value("last_path",workDir.absolutePath()).toString();
        QString filePath = QFileDialog::getSaveFileName(this->parentWidget(),
                                                        "Save planner data as...", last_path,
                                                        "Kautham Planner Solution (*.kps)");
        if (!filePath.isEmpty()) {
            if (filePath.contains(".")) filePath.truncate(filePath.lastIndexOf("."));
            filePath.append(".kps");
        }

        return filePath;
    }


    void PlannerWidget::loadData(){
        if(_planner != NULL ){
            if(_planner->getFamily()=="ioc") {
#if defined(KAUTHAM_USE_IOC)
                QString path,dir;
                QDir workDir;
                emit changeCursor(true);
                path = QFileDialog::getOpenFileName( this->parentWidget(),
                                                     "Load a file...", workDir.absolutePath(),
                                                     "Kautham Planner Solution (*.kps)");
                if(!path.isEmpty()){
                    sendText(QString("The solution file in " + path + " is being loaded.").toUtf8().constData() );
                    dir = path;
                    dir.truncate(dir.lastIndexOf("/"));
                    ((IOC::iocPlanner*)_planner)->loadData(path.toUtf8().constData());
                    if( _planner->isSolved() ) moveButton->setEnabled(true);
                }
                emit changeCursor(false);
                setTable(_planner->getParametersAsString());
#endif
            }
        } else {
            writeGUI("Sorry: Nothing implemented yet for non-ioc planners");
        }
    }


    void PlannerWidget::simulatePath() {
        if (moveButton->text() == QApplication::translate("Form", "Start Move ", 0, QApplication::UnicodeUTF8)){
            _plannerTimer->start(200);
            _ismoving = true;
            //_stepSim = 0;
            moveButton->setText(QApplication::translate("Form", "Stop Move ", 0, QApplication::UnicodeUTF8));
        } else {
            _plannerTimer->stop();
            moveButton->setText(QApplication::translate("Form", "Start Move ", 0, QApplication::UnicodeUTF8));
            _ismoving = false;
        }

    }



    void PlannerWidget::moveAlongPath(){
        _planner->moveAlongPath(_stepSim);
        // It moves the camera if the associated planner provides the
        // transformation information of the camera
        if( chkCamera->isChecked() && _planner->getCameraMovement(_stepSim) != NULL ) {
            //_gui->setActiveCameraTransform(*_planner->getCameraMovement( _stepSim ));
        }

        _stepSim += _planner->getSpeedFactor();

    }

    void PlannerWidget::showSample(int index){
        int max;

        connectLabel->setPixmap(QPixmap(QString::fromUtf8(":/icons/tryconnect.xpm")));

        max = _samples->getSize();

        if (_samples->getSize() > 1) {
            globalFromBox->setMaximum(max-1);
            globalToBox->setMaximum(max-1);
            localFromBox->setMaximum(max-1);
            localToBox->setMaximum(max-1);
        }

        if (index >= 0 && index < max) {
            Sample *smp =  _samples->getSampleAt(index);
            _planner->wkSpace()->moveRobotsTo(smp );

            vector<KthReal> c = smp->getCoords();
            cout << "sample: ";

            for(int i=0; i<c.size(); i++)
                cout << c[i] << ", ";

            cout << endl;

            if (smp->getMappedConf().size()!=0) {
                SE3Conf &s = smp->getMappedConf()[0].getSE3();
                cout << s.getPos().at(0) << " ";
                cout << s.getPos().at(1) << " ";
                cout << s.getPos().at(2) << endl;

            }

        } else {
            globalFromBox->setValue(0);
            globalToBox->setValue(0);
            localFromBox->setValue(0);
            localToBox->setValue(0);
        }
    }


    void PlannerWidget::chkCameraClick() {
        if (chkCamera->isChecked()) {
            _planner->wkSpace()->setPathVisibility(false);
        } else {
            _planner->wkSpace()->setPathVisibility(true);
        }
    }


    PlannersWidget::PlannersWidget(Planner *planner, SampleSet *sampleSet, bool setCamera,
                                   QWidget *parent, Qt::WindowFlags f):QWidget(parent,f) {
        _planner = planner;
        _samples = sampleSet;
        _stepSim = 0;
        _isMoving = false;

        QVBoxLayout *mainLayout = new QVBoxLayout();
        mainLayout->setObjectName(QString::fromUtf8("mainLayout"));
        setLayout(mainLayout);

        QGroupBox *groupBox = new QGroupBox("Local planner");
        groupBox->setObjectName(QString::fromUtf8("localPlannerGroupBox"));
        mainLayout->addWidget(groupBox);

        QGridLayout *gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("localPlannerLayout"));
        gridLayout->setContentsMargins(0,9,0,0);
        groupBox->setLayout(gridLayout);

        QLabel *label = new QLabel("From");
        label->setObjectName(QString::fromUtf8("localPlannerFromLabel"));
        gridLayout->addWidget(label,0,0);

        localFromBox = new QComboBox();
        localFromBox->setObjectName(QString::fromUtf8("localPlannerFromComboBox"));
        localFromBox->setCurrentIndex(0);
        connect(localFromBox,SIGNAL(valueChanged(int)),this,SLOT(showSample(int)));
        gridLayout->addWidget(label,0,1);

        label = new QLabel("To");
        label->setObjectName(QString::fromUtf8("localPlannerToLabel"));
        gridLayout->addWidget(label,0,3);

        localToBox = new QComboBox();
        localToBox->setObjectName(QString::fromUtf8("localPlannerToComboBox"));
        localToBox->setCurrentIndex(1);
        connect(localToBox,SIGNAL(valueChanged(int)),this,SLOT(showSample(int)));
        gridLayout->addWidget(label,0,4);

        QPushButton *button = new QPushButton("Connect");
        button->setObjectName(QString::fromUtf8("connectButton"));
        connect(button,SIGNAL(clicked()),this,SLOT(tryConnect()));
        gridLayout->addWidget(button,1,0,1,2);

        connectLabel = new QLabel();
        connectLabel->setObjectName(QString::fromUtf8("connectLabel"));
        connectLabel->setPixmap(QPixmap(":/icons/tryconnect.xpm"));
        connectLabel->setAlignment(Qt::AlignCenter);
        gridLayout->addWidget(connectLabel,1,3,1,2);

        groupBox = new QGroupBox("Global Planner");
        groupBox->setObjectName(QString::fromUtf8("globalPlannerGroupBox"));
        mainLayout->addWidget(groupBox);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("globalPlannerLayout"));
        gridLayout->setContentsMargins(0,9,0,0);
        groupBox->setLayout(gridLayout);

        globalPlannerBox = new QComboBox();
        globalPlannerBox->setObjectName(QString::fromUtf8("globalPlannerComboBox"));
        gridLayout->addWidget(globalPlannerBox,0,0,1,4);

        QIcon configureIcon;
        configureIcon.addFile(":/icons/configure_16x16.png");
        configureIcon.addFile(":/icons/configure_22x22.png");

        button = new QPushButton(configureIcon,"Configure planner");
        button->setObjectName(QString::fromUtf8("configureButton"));
        gridLayout->addWidget(button,1,0,1,4);

        label = new QLabel("From");
        label->setObjectName(QString::fromUtf8("globalPlannerFromLabel"));
        gridLayout->addWidget(label,2,0);

        globalFromBox = new QComboBox();
        globalFromBox->setObjectName(QString::fromUtf8("globalPlannerFromComboBox"));
        globalFromBox->setCurrentIndex(0);
        connect(globalFromBox, SIGNAL(valueChanged(int)),this,SLOT(showSample(int)));
        gridLayout->addWidget(label,2,1);

        label = new QLabel("To");
        label->setObjectName(QString::fromUtf8("globalPlannerToLabel"));
        gridLayout->addWidget(label,2,3);

        globalToBox = new QComboBox();
        globalToBox->setObjectName(QString::fromUtf8("globalPlannerToComboBox"));
        globalToBox->setCurrentIndex(1);
        connect(globalToBox, SIGNAL(valueChanged(int)),this,SLOT(showSample(int)));
        gridLayout->addWidget(label,2,4);

        QIcon loadIcon;
        loadIcon.addFile(":/icons/fileopen_16x16.png");
        loadIcon.addFile(":/icons/fileopen_22x22.png");

        button = new QPushButton(loadIcon,"Load");
        button->setObjectName(QString::fromUtf8("loadButton"));
        connect(button,SIGNAL(clicked()),this,SLOT(loadData()));
        gridLayout->addWidget(button,3,0,1,2);

        QIcon saveIcon;
        saveIcon.addFile(":/icons/filesave_16x16.png");
        saveIcon.addFile(":/icons/filesave_22x22.png");

        button = new QPushButton(saveIcon,"Save");
        button->setObjectName(QString::fromUtf8("saveButton"));
        connect(button,SIGNAL(clicked()),this,SLOT(saveData()));
        gridLayout->addWidget(button,3,2,1,2);

        QIcon solveIcon;
        solveIcon.addFile(":/icons/run_16x16.png");
        solveIcon.addFile(":/icons/run_22x22.png");

        button = new QPushButton(solveIcon,"Solve");
        button->setObjectName(QString::fromUtf8("solveButton"));

        gridLayout->addWidget(button,4,0,1,2);

        QIcon moveIcon;
        moveIcon.addFile(":/icons/right_16x16.png");
        moveIcon.addFile(":/icons/right_22x22.png");

        moveButton = new QPushButton(moveIcon,"Move");
        moveButton->setObjectName(QString::fromUtf8("moveButton"));
        moveButton->setDisabled(true);
        connect(moveButton,SIGNAL(clicked()),this,SLOT(simulatePath()));
        gridLayout->addWidget(moveButton,4,2,1,2);

        if (setCamera) {
            cameraCheckBox = new QCheckBox("Move camera with:");
            cameraCheckBox->setObjectName(QString::fromUtf8("camerCheckBox"));
            cameraCheckBox->setChecked(false);
            gridLayout->addWidget(cameraCheckBox,5,0,1,4);

            linkBox = new QComboBox();
            linkBox->setObjectName(QString::fromUtf8("linkBox"));
            gridLayout->addWidget(linkBox,6,0,1,4);
        }

        /*
        connect(_plannerTimer, SIGNAL(timeout()), this, SLOT(moveAlongPath()));
        connect(_cmbTry, SIGNAL( clicked() ), this, SLOT( tryConnect( )));
        connect(chkCamera, SIGNAL( clicked() ), this, SLOT( chkCameraClick( )));
        */
    }


    void PlannersWidget::getPath() {}
    void PlannersWidget::saveData() {}
    void PlannersWidget::loadData() {}
    void PlannersWidget::moveAlongPath() {}
    void PlannersWidget::showSample(int index) {}
    void PlannersWidget::tryConnect() {
        if (_planner != NULL) {
            switch ((int)_planner->getFamily()) {
#if defined(KAUTHAM_USE_IOC)
            case IOCPLANNER:
                tryConnectIOC();
                break;
#endif
#if defined(KAUTHAM_USE_OMPL)
            case OMPLPLANNER:
                tryConnectOMPL();
                break;
            case OMPLCPLANNER:
                writeGUI("Sorry: Nothing implemented yet for non-ioc planners");

                break;
#if defined(KAUTHAM_USE_ODE)
            case ODEPLANNER:
                writeGUI("Sorry: Nothing implemented yet for non-ioc planners");

                break;
#endif
#endif
            case NOFAMILY:
                writeGUI("The planner is not configured properly!!. Something is wrong with your application.");

                break;
            default:
                writeGUI("The planner is not configured properly!!. Something is wrong with your application.");

                break;
            }
        } else {
            writeGUI("The planner is not configured properly!!. Something is wrong with your application.");
        }
    }


    void PlannersWidget::tryConnectIOC() {
#if defined(KAUTHAM_USE_IOC)
        Sample *fromSample = _samples->getSampleAt(localFromBox->currentText().toInt());
        Sample *toSample = _samples->getSampleAt(localToBox->currentText().toInt());
        ((IOC::iocPlanner *)_planner)->getLocalPlanner()->setInitSamp(fromSample);
        ((IOC::iocPlanner *)_planner)->getLocalPlanner()->setGoalSamp(toSample);

        KthReal distance = ((IOC::iocPlanner*)_planner)->getLocalPlanner()->
                distance(fromSample,toSample);

        writeGUI("Distance:  "+QString::number(distance).toStdString());

        if (((IOC::iocPlanner*)_planner)->getLocalPlanner()->canConect()) {
            connectLabel->setPixmap(QPixmap(QString::fromUtf8(":/icons/connect.xpm")));
            writeGUI("The samples can be connected.");
        } else {
            connectLabel->setPixmap(QPixmap(QString::fromUtf8(":/icons/noconnect.xpm")));
            writeGUI("The samples can NOT be connected.");
        }
#endif
    }


    void PlannersWidget::tryConnectOMPL() {
#if defined(KAUTHAM_USE_OMPL)
        writeGUI("Sorry: Nothing implemented yet for non-ioc planners");
#endif
    }

    void PlannersWidget::tryConnectOMPLC() {
#if defined(KAUTHAM_USE_OMPL)
        writeGUI("Sorry: Nothing implemented yet for non-ioc planners");
#endif
    }

    void PlannersWidget::tryConnectODE() {
#if defined(KAUTHAM_USE_OMPL) && defined(KAUTHAM_USE_ODE)
        writeGUI("Sorry: Nothing implemented yet for non-ioc planners");
#endif
    }


    void PlannersWidget::writeGUI(string text){
        emit sendText(text);
    }

    void PlannersWidget::setCamera() {}

}
