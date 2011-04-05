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
 
  
#include <Inventor/Qt/SoQt.h>
#include <QFile>
#include <QString>
#include <QMessageBox>
#include <sstream>
#include <libproblem/ivworkspace.h>
#include <libdevice/device.h>
#include <libdevice/hapticdevice.h>
#include <libutil/kauthamdefs.h>
#include "application.h"

using namespace libProblem;
using namespace libPlanner;

Application::Application() { 
  Q_INIT_RESOURCE(kauthamRes);
	mainWindow = new GUI();
  SoQt::show(mainWindow);
	setActions();
	mainWindow->setText("Open a problem file to start...");
	appState = INITIAL ;
  _problem = NULL;
  _hapticLoaded = false;
  _planner = NULL;
  _localPlanner = NULL;
}

Application::~Application() {
  if(_devices.size() > 0){
    for(int i = 0; i < _devices.size();i++)
      delete _devices.at(i);
    _devices.clear();
  }
}

void Application::setActions(){
	mainWindow->setAction(FILETOOL,"&Open","CTRL+O",":/icons/fileopen.xpm",this,SLOT(openFile()));
	mainWindow->setAction(FILETOOL,"&Save","CTRL+S",":/icons/filesave.xpm",this,SLOT(saveFile()));
	mainWindow->setAction(FILETOOL,"Save &as","CTRL+A",":/icons/saveas.xpm",this,SLOT(saveAsFile()));
	mainWindow->addSeparator(TOOLBAR);

  // Creating the planner toolbar in the main Window. This list may change.
  string loc = Problem::localPlannersNames();
  string glob = Problem::plannersNames();


  mainWindow->createPlannerToolBar(loc, glob,this,SLOT(changePlanner(string,string)));
  mainWindow->setToogleAction(ACTIONTOOL,"&Find path","CTRL+F",":/icons/prm.xpm",mainWindow,SLOT(showPlannerToolBar()));
  mainWindow->addSeparator(TOOLBAR);
  mainWindow->addSeparator(ACTIONMENU);

  mainWindow->setAction(ACTIONTOOL,"Haptic connection","CTRL+U",":/icons/phantom_on.xpm",
                        this,SLOT(loadHaptic()));
  mainWindow->setAction(ACTIONTOOL,"Stop Haptic connection","CTRL+D",":/icons/phantom_off.xpm",this,SLOT(unloadHaptic()));
	mainWindow->addSeparator(TOOLBAR);
  mainWindow->setAction(ACTIONTOOL,"Chan&ge Colour","CTRL+G",
                              ":/icons/determ.xpm", mainWindow, SLOT(changeActiveBackground()));
  mainWindow->setAction(FILETOOL,"&Close","CTRL+Q",":/icons/close.xpm",this,SLOT(closeProblem()));
}

void Application::openFile(){
	QString path,dir;
	QDir workDir;
  mainWindow->setCursor(QCursor(Qt::WaitCursor));
	switch(appState){
		case INITIAL:
      path = QFileDialog::getOpenFileName(
					mainWindow,
					"Choose a file to open",
					workDir.absolutePath(),
					"All configuration files (*.xml)");
			if(!path.isEmpty()){
        
        mainWindow->setText("Kautham is opening a problem file...");
        dir = path;
        dir.truncate(dir.lastIndexOf("/"));
        problemSetup(path.toUtf8().constData());
        stringstream tmp;
        tmp << "Kautham ";
        tmp << MAJOR_VERSION;
        tmp << ".";
        tmp << MINOR_VERSION;
        tmp << " - ";
        tmp << path.toUtf8().constData();
        mainWindow->setWindowTitle( tmp.str().c_str() );
        mainWindow->setText(QString("File: ").append(path).toUtf8().constData() );
				mainWindow->setText("opened successfully.");
			}
			break;
		case PROBLEMLOADED:
			break;
		default:
			break;
	}
  mainWindow->setCursor(QCursor(Qt::ArrowCursor));
}

void Application::saveFile(){
  mainWindow->setCursor(QCursor(Qt::WaitCursor));
  if( appState == PROBLEMLOADED ){
    if( _problem->saveToFile() )
      mainWindow->setText( "File saved successfully" );
    else
      mainWindow->setText( "Sorry but the file is not saved" );
  }
  mainWindow->setCursor(QCursor(Qt::ArrowCursor));
}

void Application::saveAsFile(){
  QString path,dir;
	QDir workDir;
  mainWindow->setCursor(QCursor(Qt::WaitCursor));
	switch(appState){
		case PROBLEMLOADED:
      path = QFileDialog::getSaveFileName(
					mainWindow,
					"Save as ...",
					workDir.absolutePath(),
					"All configuration files (*.xml)");
			if(!path.isEmpty()){
        mainWindow->setText( "Kautham is saving a problem file: " );
        mainWindow->setText( path.toUtf8().constData() );
        dir = path;
        dir.truncate(dir.lastIndexOf("/"));
        if( _problem->saveToFile( path.toUtf8().constData() ) )
          mainWindow->setText( "File saved successfully" );
        else
          mainWindow->setText( "Sorry but the file is not saved" );
      }
  }
  mainWindow->setCursor(QCursor(Qt::ArrowCursor));
}

void Application::open(){
	QMessageBox::information(0, "Kautham 2.0","Is calling an Open procedure.");
}

void Application::closeProblem(){
  mainWindow->setCursor(QCursor(Qt::WaitCursor));
  switch(appState){
  case INITIAL:
    mainWindow->setText("First open a problem");
    break;
  case PROBLEMLOADED:
    mainWindow->restart();
    delete _problem;
    appState = INITIAL;
    break;
  }
  mainWindow->setCursor(QCursor(Qt::ArrowCursor));
}

void Application::changePlanner(string loc, string glob){
  mainWindow->setCursor(QCursor(Qt::WaitCursor));
  if(_problem != NULL){
    if(_problem->getPlanner() != NULL){
      string name = _problem->getPlanner()->getGuiName();
      mainWindow->removePropTab(name);
    }

    _problem->createLocalPlanner(loc);
    _problem->createPlanner(glob);

    mainWindow->addPlanner(_problem->getPlanner(), _problem->getSampleSet(), mainWindow);
  }
  mainWindow->setCursor(QCursor(Qt::ArrowCursor));
}

void Application::loadHaptic(){
  if( appState == INITIAL )return;
  mainWindow->setCursor(QCursor(Qt::WaitCursor));
  if(_hapticLoaded == false){
    Device* tmpDev = new HapticDevice("Haptic",10);
    _devices.push_back(tmpDev);
    mainWindow->addDevice(tmpDev, 50);
    mainWindow->addTeleoperationWidget(_problem, ((HapticDevice*)tmpDev));
    _hapticLoaded = true;
  }else
    mainWindow->setText("Use the tab provided to drive the Haptic");

  mainWindow->setCursor(QCursor(Qt::ArrowCursor));
}

void Application::unloadHaptic(){
  mainWindow->setCursor(QCursor(Qt::WaitCursor));
  if(_hapticLoaded == false){
    mainWindow->setText("You do not have the Haptic connection loaded");
  }else{
    _hapticLoaded = false;
    
    for(int i=0; i < _devices.size(); i++)
      if(((Device*)_devices.at(i))->getGuiName() == "Haptic"){
        delete _devices.at(i);
        _devices.erase(_devices.begin()+i);
        //vector<Device*>::iterator it = _devices.erase(_devices.begin()+i);
        //delete (*it);
      }
    mainWindow->removePropTab("Teleoperation");
    mainWindow->removePropTab("Haptic");
  }
  mainWindow->setCursor(QCursor(Qt::ArrowCursor));
}

bool Application::problemSetup(string path){
  mainWindow->setCursor(QCursor(Qt::WaitCursor));
  _problem = new Problem();
  _problem->setupFromFile( path );

  mainWindow->addToProblemTree( path );
  mainWindow->addViewerTab("WSpace", libGUI::SPACE, ((IVWorkSpace*)_problem->wSpace())->getIvScene());

  //  Using to show the IV models reconstructed from the PQP triangular meshes.
  //mainWindow->addViewerTab("PQP", libGUI::SPACE, ((IVWorkSpace*)_problem->wSpace())->getIvFromPQPScene());

  int globOffset = 0;
  for(unsigned i = 0; i < _problem->wSpace()->robotsCount(); i++){
    mainWindow->addControlWidget(_problem->wSpace()->getRobot(i), _problem, globOffset);
	if(_problem->wSpace()->getRobot(i)->getCkine() != NULL)
		//mainWindow->addConstrainedControlWidget(_problem->wSpace()->getRobot(i), _problem, globOffset);
    // Use the following widget if the user can modified all the dof instead of the controls.
    //mainWindow->addDOFWidget(_problem->wSpace()->getRobot(i) );

    mainWindow->addBronchoWidget(_problem->wSpace()->getRobot(i), _problem, globOffset, mainWindow);

    if(_problem->wSpace()->getRobot(i)->getIkine() != NULL)
      mainWindow->addInverseKinematic(_problem->wSpace()->getRobot(i)->getIkine());
      globOffset +=  _problem->wSpace()->getRobot(i)->getNumControls();
  }

  mainWindow->setSampleWidget(_problem->getSampleSet(), _problem->getSampler(), _problem);

  if( _problem->getPlanner() != NULL )
    mainWindow->addPlanner(_problem->getPlanner(), _problem->getSampleSet(), mainWindow);

  appState = PROBLEMLOADED;
  mainWindow->setCursor(QCursor(Qt::ArrowCursor));
  return true;
}


