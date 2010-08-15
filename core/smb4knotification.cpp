/***************************************************************************
    smb4knotification  -  This class provides notifications for Smb4K.
                             -------------------
    begin                : Son Jun 27 2010
    copyright            : (C) 2010 by Alexander Reinholdt
    email                : alexander.reinholdt@kdemail.org
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
 *   Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,   *
 *   MA  02111-1307 USA                                                    *
 ***************************************************************************/

// KDE includes
#include <kdebug.h>
#include <klocale.h>
#include <kiconloader.h>
#include <krun.h>
#include <kurl.h>
#include <kmessagebox.h>

// application specific includes
#include <smb4knotification.h>
#include <smb4kshare.h>
#include <smb4ksettings.h>


Smb4KNotification::Smb4KNotification( QObject *parent )
: QObject( parent )
{
}


Smb4KNotification::~Smb4KNotification()
{
}


void Smb4KNotification::shareMounted( Smb4KShare* share )
{
  Q_ASSERT( share );
  
  if ( Smb4KSettings::self()->showNotifications() )
  {
    m_share = *share;
  
    KNotification *notification = new KNotification( "shareMounted", KNotification::CloseOnTimeout );
    notification->setText( i18n( "The share <b>%1</b> has been mounted to <b>%2</b>." ).arg( share->unc() )
                          .arg( QString::fromUtf8( share->canonicalPath() ) ) );
    notification->setActions( QStringList( i18n( "Open" ) ) );
    notification->setPixmap( KIconLoader::global()->loadIcon( "folder-remote", KIconLoader::NoGroup, 0, KIconLoader::DefaultState, QStringList( "emblem-mounted" ) ) );
    
    connect( notification, SIGNAL( activated( unsigned int ) ), this, SLOT( slotOpenShare() ) );
    connect( notification, SIGNAL( closed() ), this, SLOT( slotNotificationClosed() ) );

    notification->sendEvent();
  }
  else
  {
    // Do nothing
  }
}


void Smb4KNotification::shareUnmounted( Smb4KShare* share )
{
  Q_ASSERT( share );
  
  if ( Smb4KSettings::self()->showNotifications() )
  {  
    KNotification *notification = KNotification::event( "shareUnmounted", 
                                  i18n( "The share <b>%1</b> has been unmounted from <b>%2</b>." ).arg( share->unc() ).arg( QString::fromUtf8( share->path() ) ),
                                  KIconLoader::global()->loadIcon( "folder-remote", KIconLoader::NoGroup, 0, KIconLoader::DefaultState, QStringList( "emblem-unmounted" ) ) );
    connect( notification, SIGNAL( closed() ), this, SLOT( slotNotificationClosed() ) );
  }
  else
  {
    // Do nothing
  }
}


void Smb4KNotification::sharesRemounted( int total, int actual )
{
  if ( Smb4KSettings::self()->showNotifications() )
  {
    if ( total != actual )
    {
      KNotification *notification = KNotification::event( "sharesRemounted", 
                                    i18np( "%1 share out of %2 has been remounted.", "%1 shares out of %2 have been remounted.", actual, total ),
                                    KIconLoader::global()->loadIcon( "folder-remote", KIconLoader::NoGroup, 0, KIconLoader::DefaultState, QStringList( "emblem-mounted" ) ) );
      connect( notification, SIGNAL( closed() ), this, SLOT( slotNotificationClosed() ) );
    }
    else
    {
      KNotification *notification = KNotification::event( "sharesRemounted", i18n( "All shares have been remounted." ),
                                    KIconLoader::global()->loadIcon( "folder-remote", KIconLoader::NoGroup, 0, KIconLoader::DefaultState, QStringList( "emblem-mounted" ) ) );
      connect( notification, SIGNAL( closed() ), this, SLOT( slotNotificationClosed() ) );
    }
  }
  else
  {
    // Do nothing
  }
}


void Smb4KNotification::allSharesUnmounted( int total, int actual )
{
  if ( Smb4KSettings::self()->showNotifications() )
  {
    if ( total != actual )
    {
      KNotification *notification = KNotification::event( "allSharesUnmounted", 
                                    i18np( "%1 share out of %2 has been unmounted.", "%1 shares out of %2 have been unmounted.", actual, total ),
                                    KIconLoader::global()->loadIcon( "folder-remote", KIconLoader::NoGroup, 0, KIconLoader::DefaultState, QStringList( "emblem-unmounted" ) ) );
      connect( notification, SIGNAL( closed() ), this, SLOT( slotNotificationClosed() ) );
    }
    else
    {
      KNotification *notification = KNotification::event( "allSharesUnmounted", i18n( "All shares have been unmounted." ),
                                    KIconLoader::global()->loadIcon( "folder-remote", KIconLoader::NoGroup, 0, KIconLoader::DefaultState, QStringList( "emblem-unmounted" ) ) );
      connect( notification, SIGNAL( closed() ), this, SLOT( slotNotificationClosed() ) );
    }
  }
  else
  {
    // Do nothing
  }
}


//
// Warnings
//


void Smb4KNotification::openingWalletFailed( const QString &name )
{
  KNotification *notification = KNotification::event( "openingWalletFailed", 
                                i18n( "Opening the wallet <b>%1</b> failed." ).arg( name ),
                                KIconLoader::global()->loadIcon( "dialog-warning", KIconLoader::NoGroup, 0, KIconLoader::DefaultState ) );
  connect( notification, SIGNAL( closed() ), this, SLOT( slotNotificationClosed() ) );
}


void Smb4KNotification::loginsNotAccessible()
{
  KNotification *notification = KNotification::event( "loginsNotAccessible", 
                                i18n( "The logins stored in the wallet could not be accessed. There is either no wallet available or it could not be opened." ),
                                KIconLoader::global()->loadIcon( "dialog-warning", KIconLoader::NoGroup, 0, KIconLoader::DefaultState ) );
  connect( notification, SIGNAL( closed() ), this, SLOT( slotNotificationClosed() ) );
}


void Smb4KNotification::sudoNotFound()
{
  KNotification *notification = KNotification::event( "sudoNotFound",
                                i18n( "The program <b>sudo</b> could not be found. Smb4K will continue without using it." ),
                                KIconLoader::global()->loadIcon( "dialog-warning", KIconLoader::NoGroup, 0, KIconLoader::DefaultState ) );
  connect( notification, SIGNAL( closed() ), this, SLOT( slotNotificationClosed() ) );
}


void Smb4KNotification::kdesudoNotFound()
{
  KNotification *notification = KNotification::event( "kdesudoNotFound",
                                i18n( "The program <b>kdesudo</b> could not be found. Smb4K will continue with using <b>sudo</b> instead." ),
                                KIconLoader::global()->loadIcon( "dialog-warning", KIconLoader::NoGroup, 0, KIconLoader::DefaultState ) );
  connect( notification, SIGNAL( closed() ), this, SLOT( slotNotificationClosed() ) );
}


//
// Errors
//


void Smb4KNotification::retrievingDomainsFailed( const QString &err_msg )
{
  m_err_msg = err_msg.split( "\n" );
  
  KNotification *notification = new KNotification( "retrievingDomainsFailed", KNotification::Persistent );
  notification->setText( i18n( "Retrieving the list of domains and workgroups failed." ) );
  notification->setActions( QStringList( i18n( "Error Message" ) ) );
  notification->setPixmap( KIconLoader::global()->loadIcon( "dialog-error", KIconLoader::NoGroup, 0, KIconLoader::DefaultState ) );
  
  connect( notification, SIGNAL( activated( unsigned int ) ), this, SLOT( slotShowErrorMessage() ) );
  connect( notification, SIGNAL( closed() ), this, SLOT( slotNotificationClosed() ) );
  
  notification->sendEvent();
}


/////////////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATIONS
/////////////////////////////////////////////////////////////////////////////

void Smb4KNotification::slotNotificationClosed()
{
  delete this;
}


void Smb4KNotification::slotOpenShare()
{
  KRun::runUrl( KUrl( m_share.canonicalPath() ), "inode/directory", 0 );
}


void Smb4KNotification::slotShowErrorMessage()
{
  KMessageBox::errorList( 0, i18n( "The following error message was reported:" ), m_err_msg );
}



#include "smb4knotification.moc"
