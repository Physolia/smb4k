/***************************************************************************
    smb4kprofilesmenu  -  The menu for the profiles
                             -------------------
    begin                : Do Aug 10 2014
    copyright            : (C) 2014 by Alexander Reinholdt
    email                : alexander.reinholdt@kdemail.net
 ***************************************************************************/

/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   General Public License for more details.                              *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc., 51 Franklin Street, Suite 500, Boston,*
 *   MA 02110-1335, USA                                                    *
 ***************************************************************************/

#ifndef SMB4KPROFILESMENU_H
#define SMB4KPROFILESMENU_H

// KDE includes
#include <kselectaction.h>


class Smb4KProfilesMenu : public KSelectAction
{
  Q_OBJECT
  
  public:
    /**
     * Constructor
     */
    explicit Smb4KProfilesMenu(QObject* parent);
    
    /**
     * Destructor
     */
    virtual ~Smb4KProfilesMenu();
    
  protected Q_SLOTS:
    void slotSettingsChanged();
    void slotActionTriggered(const QString &name);
    
  private:
    
};

#endif
