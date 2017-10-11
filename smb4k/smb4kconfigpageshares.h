/***************************************************************************
    The configuration page for the settings regarding share management
                             -------------------
    begin                : Sa Nov 15 2003
    copyright            : (C) 2003-2017 by Alexander Reinholdt
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

#ifndef SMB4KCONFIGPAGESHARES_H
#define SMB4KCONFIGPAGESHARES_H

// Qt includes
#include <QWidget>

/**
 * This is the configuration tab for the settings that are
 * used to manage the mounted shares.
 *
 * @author Alexander Reinholdt <alexander.reinholdt@kdemail.net>
 */

class Smb4KConfigPageShares : public QWidget
{
  Q_OBJECT

  public:
    /**
     * The constructor.
     *
     * @param parent          The parent of this widget
     */
    explicit Smb4KConfigPageShares(QWidget *parent = 0);

    /**
     * The destructor.
     */
    ~Smb4KConfigPageShares();
};
#endif
