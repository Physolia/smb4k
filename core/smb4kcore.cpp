/***************************************************************************
    smb4kcore  -  The main core class of Smb4K.
                             -------------------
    begin                : Do Apr 8 2004
    copyright            : (C) 2004-2009 by Alexander Reinholdt
    email                : dustpuppy@users.berlios.de
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
 *   Free Software Foundation, 51 Franklin Street, Suite 500, Boston,      *
 *   MA 02110-1335, USA                                                    *
 ***************************************************************************/

// Qt includes
#include <QDir>
#include <QMap>
#include <QStringList>
#include <QDesktopWidget>
#include <QHostAddress>

// KDE includes
#include <klocale.h>
#include <kurl.h>
#include <krun.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kshell.h>
#include <kstandarddirs.h>
#include <kglobal.h>

#include <kmessagebox.h>

// system includes
#include <stdlib.h>

// application specific includes
#include <smb4kcore.h>
#include <smb4kglobal.h>
#include <smb4ksettings.h>
#include <smb4kshare.h>
#include <smb4kscanner.h>
#include <smb4kmounter.h>
#include <smb4kprint.h>
#include <smb4ksynchronizer.h>
#include <smb4kpreviewer.h>
#include <smb4ksearch.h>
#include <smb4knotification.h>


using namespace Smb4KGlobal;

// FIXME: Move to own files later.

class Smb4KCorePrivate
{
  public:
    /**
     * The Smb4KCore instance
     */
    Smb4KCore instance;
};

K_GLOBAL_STATIC( Smb4KCorePrivate, m_priv );


Smb4KCore::Smb4KCore() : QObject()
{
  // Set default values for settings that depend on the system
  // Smb4K is running on:
  setDefaultSettings();

  // Search for the programs that are needed by Smb4K:
  searchPrograms();

  connect( kapp,           SIGNAL( aboutToQuit() ),
           this,           SLOT( slotAboutToQuit() ) );
}


Smb4KCore::~Smb4KCore()
{
  // Do not call abort() here. This will most likely lead
  // to crashes.
}


Smb4KCore *Smb4KCore::self()
{
  return &m_priv->instance;
}


/****************************************************************************
   Returns a bool that tells the program whether a core process is running.
****************************************************************************/

bool Smb4KCore::isRunning()
{
  return (Smb4KScanner::self()->isRunning() ||
          Smb4KMounter::self()->isRunning() ||
          Smb4KPrint::self()->isRunning() ||
          Smb4KSynchronizer::self()->isRunning() ||
          Smb4KPreviewer::self()->isRunning() ||
          Smb4KSearch::self()->isRunning());
}


/****************************************************************************
   Aborts any process of the core.
****************************************************************************/

void Smb4KCore::abort()
{
  Smb4KScanner::self()->abortAll();
  Smb4KMounter::self()->abortAll();
  Smb4KPrint::self()->abortAll();
  Smb4KSynchronizer::self()->abortAll();
  Smb4KPreviewer::self()->abortAll();
  Smb4KSearch::self()->abortAll();
}


void Smb4KCore::init()
{
  Smb4KScanner::self()->start();
  Smb4KMounter::self()->start();
}


void Smb4KCore::searchPrograms()
{
  // Remove the group "Programs" and reread the
  // configuration:
  Smb4KSettings::self()->config()->deleteGroup( "Programs" );
  Smb4KSettings::self()->readConfig();

  QStringList missing;
  QString program;

  // Find mandatory executables
  QStringList paths;

  if ( KStandardDirs::findExe( "grep" ).isEmpty() )
  {
    missing << "grep";
  }
  else
  {
    // Do nothing
  }

  if ( KStandardDirs::findExe( "awk" ).isEmpty() )
  {
    missing << "awk";
  }
  else
  {
    // Do nothing
  }

  if ( KStandardDirs::findExe( "sed" ).isEmpty() )
  {
    missing << "sed";
  }
  else
  {
    // Do nothing
  }

  if ( KStandardDirs::findExe( "xargs" ).isEmpty() )
  {
    missing << "xargs";
  }
  else
  {
    // Do nothing
  }

  if ( KStandardDirs::findExe( "nmblookup" ).isEmpty() )
  {
    missing << "nmblookup";
  }
  else
  {
    // Do nothing
  }

  if ( KStandardDirs::findExe( "smbclient" ).isEmpty() )
  {
    missing << "smbclient";
  }
  else
  {
    // Do nothing
  }

  if ( KStandardDirs::findExe( "smbspool" ).isEmpty() )
  {
    missing << "smbspool";
  }
  else
  {
    // Do nothing
  }

  if ( KStandardDirs::findExe( "smbtree" ).isEmpty() )
  {
    missing << "smbtree";
  }
  else
  {
    // Do nothing
  }

  if ( KStandardDirs::findExe( "net" ).isEmpty() )
  {
    missing << "net";
  }
  else
  {
    // Do nothing
  }

#ifndef Q_OS_FREEBSD
  if ( KStandardDirs::findExe( "mount.cifs" ).isEmpty() &&
       KStandardDirs::findExe( "mount.cifs", "/sbin" ).isEmpty() &&
       KStandardDirs::findExe( "mount.cifs", "/usr/sbin" ).isEmpty() &&
       KStandardDirs::findExe( "mount.cifs", "/usr/local/sbin" ).isEmpty() )
  {
    missing << "mount.cifs";
  }
  else
  {
    // Do nothing
  }
#else
  if ( KStandardDirs::findExe( "mount_smbfs" ).isEmpty() &&
       KStandardDirs::findExe( "mount_smbfs", "/sbin" ).isEmpty() &&
       KStandardDirs::findExe( "mount_smbfs", "/usr/sbin" ).isEmpty() &&
       KStandardDirs::findExe( "mount_smbfs", "/usr/local/sbin" ).isEmpty() )
  {
    missing << "mount_smbfs";
  }
  else
  {
    // Do nothing
  }
  
  if ( KStandardDirs::findExe( "smbutil" ).isEmpty() &&
       KStandardDirs::findExe( "smbutil", "/sbin" ).isEmpty() &&
       KStandardDirs::findExe( "smbutil", "/usr/sbin" ).isEmpty() &&
       KStandardDirs::findExe( "smbutil", "/usr/local/sbin" ).isEmpty() )
  {
    missing << "smbutil";
  }
  else
  {
    // Do nothing
  }
#endif
  
  if ( KStandardDirs::findExe( "umount" ).isEmpty() &&
       KStandardDirs::findExe( "umount", "/sbin" ).isEmpty() &&
       KStandardDirs::findExe( "umount", "/usr/sbin" ).isEmpty() &&
       KStandardDirs::findExe( "umount", "/usr/local/sbin" ).isEmpty() )
  {
    missing << "umount";
  }
  else
  {
    // Do nothing
  }  

  if ( !missing.isEmpty() )
  {
    // Error out if one of the mandatory programs is missing:
    Smb4KNotification *notification = new Smb4KNotification();
    notification->missingPrograms( missing );
  }
  else
  {
    // Write the configuration to disk:
    Smb4KSettings::self()->writeConfig();
  }
}


void Smb4KCore::setDefaultSettings()
{
  // Samba options that have to be dynamically imported from smb.conf:
  QMap<QString, QString> opts = globalSambaOptions( true );

  if ( !opts["netbios name"].isEmpty() )
  {
    Smb4KSettings::self()->netBIOSNameItem()->setDefaultValue( opts["netbios name"] );

    if ( Smb4KSettings::netBIOSName().isEmpty() )
    {
      Smb4KSettings::self()->netBIOSNameItem()->setDefault();
    }
  }

  if ( !opts["workgroup"].isEmpty() )
  {
    Smb4KSettings::self()->domainNameItem()->setDefaultValue( opts["workgroup"] );

    if ( Smb4KSettings::domainName().isEmpty() )
    {
      Smb4KSettings::self()->domainNameItem()->setDefault();
    }
  }

  if ( !opts["socket options"].isEmpty() )
  {
    Smb4KSettings::self()->socketOptionsItem()->setDefaultValue( opts["socket options"] );

    if ( Smb4KSettings::socketOptions().isEmpty() )
    {
      Smb4KSettings::self()->socketOptionsItem()->setDefault();
    }
  }

  if ( !opts["netbios scope"].isEmpty() )
  {
    Smb4KSettings::self()->netBIOSScopeItem()->setDefaultValue( opts["netbios scope"] );

    if ( Smb4KSettings::netBIOSScope().isEmpty() )
    {
      Smb4KSettings::self()->netBIOSScopeItem()->setDefault();
    }
  }

  if ( !opts["name resolve order"].isEmpty() )
  {
    Smb4KSettings::self()->nameResolveOrderItem()->setDefaultValue( opts["name resolve order"] );

    if ( Smb4KSettings::nameResolveOrder().isEmpty() )
    {
      Smb4KSettings::self()->nameResolveOrderItem()->setDefault();
    }
  }

  QHostAddress address( opts["interfaces"].section( " ", 0, 0 ) );

  if ( address.protocol() != QAbstractSocket::UnknownNetworkLayerProtocol )
  {
    Smb4KSettings::self()->broadcastAddressItem()->setDefaultValue( address.toString() );

    if ( Smb4KSettings::broadcastAddress().isEmpty() )
    {
      Smb4KSettings::self()->broadcastAddressItem()->setDefault();
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
//  SLOT IMPLEMENTATIONS
/////////////////////////////////////////////////////////////////////////////

void Smb4KCore::slotAboutToQuit()
{
  Smb4KSettings::self()->writeConfig();
}

#include "smb4kcore.moc"
