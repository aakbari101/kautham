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

/* Author: Nestor Garcia Hidalgo */


#include <QtGui>


namespace Kautham {
/** \addtogroup Application
 *  @{
 */

    /*!
    * \brief The DefaultPathDialog class allows the user define a list of directories where
    robot and obstacle models, as well as any file required to open a problem, will be looked for.
    The directories will be consulted while the files are not found, so the order is important
    */
    class DefaultPathDialog : public QObject {
        Q_OBJECT

    public:
        /*!
         * \brief DefaultPathDialog Constructor
         * \param pathList Path list to fill the dialog
         * \param parent Parent of the dialog
         */
        DefaultPathDialog(QStringList pathList, QWidget *parent = 0);

        /*!
         * \brief exec Executes the dialog
         * \param pathList Path list defined by the user, NULL is the dialog was rejected by the user
         * \return True if the dialog was accepted
         */
        bool exec(QStringList *pathList);

    private slots:
        /*!
         * \brief addDirectory Opens a file dialog and if a valid folder is selected it will be added to the list
         */
        void addDirectory();

        /*!
         * \brief removeDirectory Removes the current selected directory from the list
         */
        void removeDirectory();

        /*!
         * \brief upDirectory Moves up the current selected directory in the list
         */
        void upDirectory();

        /*!
         * \brief downDirectory Moves down the current selected directory in the list
         */
        void downDirectory();

    private:
        QDialog *defaultPathDialog;
        QVBoxLayout *mainLayout;
        QHBoxLayout *topLayout;
        QListWidget *pathListWidget;
        QFrame *buttonFrame;
        QVBoxLayout *buttonLayout;
        QPushButton *addButton;
        QPushButton *removeButton;
        QPushButton *clearButton;
        QSpacerItem *VSpacer;
        QPushButton *upButton;
        QPushButton *downButton;
        QDialogButtonBox *buttonBox;
    };
/** @}   end of Doxygen module "Application" */
}
