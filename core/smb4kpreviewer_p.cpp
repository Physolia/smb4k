/***************************************************************************
    smb4kpreviewer_p  -  Private helper classes for Smb4KPreviewer class.
                             -------------------
    begin                : So Dez 21 2008
    copyright            : (C) 2008-2010 by Alexander Reinholdt
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
 *   Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,   *
 *   MA  02111-1307 USA                                                    *
 ***************************************************************************/

// Qt includes
#include <QTimer>
#include <QGridLayout>
#include <QDateTime>

// KDE includes
#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kmimetype.h>
#include <ktoolbar.h>
#include <kstatusbar.h>

// application specific includes
#include <smb4kpreviewer_p.h>
#include <smb4knotification.h>
#include <smb4khost.h>
#include <smb4kshare.h>
#include <smb4kglobal.h>
#include <smb4ksettings.h>
#include <smb4khomesshareshandler.h>
#include <smb4ksambaoptionshandler.h>
#include <smb4ksambaoptionsinfo.h>

using namespace Smb4KGlobal;


Smb4KPreviewJob::Smb4KPreviewJob( QObject *parent ) : KJob( parent ),
  m_started( false ), m_share( NULL ), m_parent_widget( NULL ), m_proc( NULL )
{
}


Smb4KPreviewJob::~Smb4KPreviewJob()
{
}


void Smb4KPreviewJob::start()
{
  m_started = true;
  QTimer::singleShot( 0, this, SLOT( slotStartPreview() ) );
}


void Smb4KPreviewJob::setupPreview( Smb4KShare *share, const QUrl &url, QWidget *parent )
{
  Q_ASSERT( share );
  m_share = share;
  m_url   = url;
  m_parent_widget = parent;
}


bool Smb4KPreviewJob::doKill()
{
  if ( m_proc && (m_proc->state() == KProcess::Running || m_proc->state() == KProcess::Starting) )
  {
    m_proc->abort();
  }
  else
  {
    // Do nothing
  }

  return KJob::doKill();
}


void Smb4KPreviewJob::slotStartPreview()
{
  // Find the smbclient program
  QString smbclient = KStandardDirs::findExe( "smbclient" );

  if ( smbclient.isEmpty() )
  {
    Smb4KNotification *notification = new Smb4KNotification();
    notification->commandNotFound( "smbclient" );
    emitResult();
    return;
  }
  else
  {
    // Go ahead
  }

  // Process homes shares.
  if( m_share->isHomesShare() )
  {
    if ( !Smb4KHomesSharesHandler::self()->specifyUser( m_share, m_parent_widget ) )
    {
      return;
    }
    else
    {
      // Do nothing
    }
  }
  else
  {
    // Do nothing
  }

  // Start the preview process
  emit aboutToStart( m_share, m_url );

  // Register the job with the job tracker
  jobTracker()->registerJob( this );
  connect( this, SIGNAL( result( KJob * ) ), jobTracker(), SLOT( unregisterJob( KJob * ) ) );

  emit description( this, i18n( "Acquiring Preview" ),
                    qMakePair( i18n( "Location" ), m_url.toString( QUrl::RemoveUserInfo|QUrl::RemovePort|QUrl::RemoveScheme ) ) );

  emitPercent( 0, 1 );

  // Get the path that has to be listed.
  QString path = m_url.toString( QUrl::RemoveScheme|QUrl::RemoveUserInfo|QUrl::RemovePort );
  path.remove( m_share->unc(), Qt::CaseInsensitive );

  // Compile the command line arguments
  QStringList arguments;

  // UNC
  arguments << m_share->unc();

  // Workgroup
  arguments << QString( "-W '%1'" ).arg( m_share->workgroupName() );

  // Directory
  arguments << QString( "-D '%1'" ).arg( path.isEmpty() ? "/" : path );

  // Command to perform (here: ls)
  arguments << QString( "-c '%1'" ).arg( "ls" );

  // IP address
  if ( !m_share->hostIP().isEmpty() )
  {
    arguments << QString( "-I %1" ).arg( m_share->hostIP() );
  }
  else
  {
    // Do nothing
  }

  // Machine account
  if ( Smb4KSettings::machineAccount() )
  {
    arguments << "-P";
  }
  else
  {
    // Do nothing
  }

  // Signing state
  switch ( Smb4KSettings::signingState() )
  {
    case Smb4KSettings::EnumSigningState::None:
    {
      break;
    }
    case Smb4KSettings::EnumSigningState::On:
    {
      arguments << "-S on";
      break;
    }
    case Smb4KSettings::EnumSigningState::Off:
    {
      arguments << "-S off";
      break;
    }
    case Smb4KSettings::EnumSigningState::Required:
    {
      arguments << "-S required";
      break;
    }
    default:
    {
      break;
    }
  }

  // Buffer size
  if ( Smb4KSettings::bufferSize() != 65520 )
  {
    arguments << QString( "-b %1" ).arg( Smb4KSettings::bufferSize() );
  }
  else
  {
    // Do nothing
  }

  // Get global Samba and custom options
  QMap<QString,QString> samba_options = Smb4KSambaOptionsHandler::self()->globalSambaOptions();
  Smb4KSambaOptionsInfo *info = Smb4KSambaOptionsHandler::self()->findItem( m_share );

  // Port
  if ( info && info->smbPort() != -1 )
  {
    arguments << QString( "-p %1" ).arg( info->smbPort() );
  }
  else
  {
    arguments << QString( "-p %1" ).arg( Smb4KSettings::remoteSMBPort() );
  }

  // Kerberos
  if ( info )
  {
    switch ( info->useKerberos() )
    {
      case Smb4KSambaOptionsInfo::UseKerberos:
      {
        arguments << " -k";
        break;
      }
      case Smb4KSambaOptionsInfo::NoKerberos:
      {
        // No kerberos
        break;
      }
      case Smb4KSambaOptionsInfo::UndefinedKerberos:
      {
        if ( Smb4KSettings::useKerberos() )
        {
          arguments << "-k";
        }
        else
        {
          // Do nothing
        }
        break;
      }
      default:
      {
        break;
      }
    }
  }
  else
  {
    if ( Smb4KSettings::useKerberos() )
    {
      arguments << "-k";
    }
    else
    {
      // Do nothing
    }
  }

  // Resolve order
  if ( !Smb4KSettings::nameResolveOrder().isEmpty() &&
        QString::compare( Smb4KSettings::nameResolveOrder(), samba_options["name resolve order"] ) != 0 )
  {
    arguments << QString( "-R %1" ).arg( Smb4KSettings::nameResolveOrder() );
  }
  else
  {
    // Do nothing
  }

  // NetBIOS name
  if ( !Smb4KSettings::netBIOSName().isEmpty() &&
       QString::compare( Smb4KSettings::netBIOSName(), samba_options["netbios name"] ) != 0 )
  {
    arguments << QString( "-n %1" ).arg( Smb4KSettings::netBIOSName() );
  }
  else
  {
    // Do nothing
  }

  // NetBIOS scope
  if ( !Smb4KSettings::netBIOSScope().isEmpty() &&
       QString::compare( Smb4KSettings::netBIOSScope(), samba_options["netbios scope"] ) != 0 )
  {
    arguments << QString( "-i %1" ).arg( Smb4KSettings::netBIOSScope() );
  }
  else
  {
    // Do nothing
  }

  // Socket options
  if ( !Smb4KSettings::socketOptions().isEmpty() &&
       QString::compare( Smb4KSettings::socketOptions(), samba_options["socket options"] ) != 0 )
  {
    arguments << QString( "-O %1" ).arg( Smb4KSettings::socketOptions() );
  }
  else
  {
    // Do nothing
  }

  if ( !m_share->login().isEmpty() )
  {
    arguments << QString( "-U %1" ).arg( m_share->login() );
  }
  else
  {
    arguments << "-U %";
  }

  // For the time being, we need to use the shell command stuff here,
  // because I haven't found out yet how to properly redirect the output
  // of smbclient.
  QString command;
  command += smbclient;
  command += " ";
  command += arguments.join( " " );

  m_proc = new Smb4KProcess( Smb4KProcess::Preview, this );
  m_proc->setOutputChannelMode( KProcess::SeparateChannels );
  m_proc->setEnv( "PASSWD", m_share->password(), true );
  m_proc->setShellCommand( command );

  connect( m_proc, SIGNAL( readyReadStandardOutput() ), SLOT( slotReadStandardOutput() ) );
  connect( m_proc, SIGNAL( readyReadStandardError() ),  SLOT( slotReadStandardError() ) );
  connect( m_proc, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( slotProcessFinished( int, QProcess::ExitStatus ) ) );

  m_proc->start();
}


void Smb4KPreviewJob::slotReadStandardOutput()
{
  QStringList list = QString::fromUtf8( m_proc->readAllStandardOutput(), -1 ).split( "\n", QString::SkipEmptyParts );
  QList<Item> items;

  foreach ( const QString &line, list )
  {
    if ( line.contains( "blocks of size" ) || line.contains( "Domain=[" ) )
    {
      continue;
    }
    else if ( line.contains( "NT_STATUS_ACCESS_DENIED", Qt::CaseSensitive ) ||
              line.contains( "NT_STATUS_LOGON_FAILURE", Qt::CaseSensitive ) )
    {
      // This might happen if a directory cannot be accessed due to missing
      // read permissions.
      emit authError( this );
      break;
    }
    else
    {
      QString entry = line;
      
      QString left = entry.trimmed().section( "     ", 0, -2 ).trimmed();
      QString right = entry.remove( left );

      QString name = left.section( "  ", 0, -2 ).trimmed().isEmpty() ?
                     left :
                     left.section( "  ", 0, -2 ).trimmed();

      QString dir_string = left.right( 3 ).trimmed();
      bool is_dir = (!dir_string.isEmpty() && dir_string.contains( "D" ));

      QString tmp_size = right.trimmed().section( "  ", 0, 0 ).trimmed();
      QString size;

      if ( tmp_size[0].isLetter() )
      {
        size = right.trimmed().section( "  ", 1, 1 ).trimmed();
      }
      else
      {
        size = tmp_size;
      }

      QString date = QDateTime::fromString( right.section( QString( " %1 " ).arg( size ), 1, 1 ).trimmed() ).toString();

      if ( !name.isEmpty() )
      {
        Item item;

        if ( is_dir )
        {
          if ( name.startsWith( "." ) &&
              (QString::compare( name, "." ) != 0 && QString::compare( name, ".." ) != 0) )
          {
            item.first = HiddenDirectoryItem;
          }
          else
          {
            item.first = DirectoryItem;
          }
        }
        else
        {
          if ( name.startsWith( "." ) )
          {
            item.first = HiddenFileItem;
          }
          else
          {
            item.first = FileItem;
          }
        }

        item.second["name"] = name;
        item.second["size"] = size;
        item.second["date"] = date;

        items << item;
      }
      else
      {
        continue;
      }
    }
  }

  emit preview( m_url, items );
}


void Smb4KPreviewJob::slotReadStandardError()
{
  QString stderr = QString::fromUtf8( m_proc->readAllStandardError(), -1 ).trimmed();

  // Remove DEBUG messages and the additional information
  // that smbclient unfortunately reports to stderr.
  QStringList err_msg = stderr.split( "\n", QString::SkipEmptyParts );

  QMutableStringListIterator it( err_msg );

  while ( it.hasNext() )
  {
    QString line = it.next();

    if ( line.contains( "DEBUG" ) )
    {
      it.remove();
    }
    else if ( line.trimmed().startsWith( "Domain" ) || line.trimmed().startsWith( "OS" ) )
    {
      it.remove();
    }
    else
    {
      // Do nothing
    }
  }

  // Avoid reporting an error if the process was killed by calling the abort() function
  // or if only debug and other information was reported.
  if ( !m_proc->isAborted() && !err_msg.isEmpty() )
  {
    m_proc->abort();

    if ( stderr.contains( "NT_STATUS_LOGON_FAILURE" ) ||
         stderr.contains( "NT_STATUS_ACCESS_DENIED" ) )
    {
      // Authentication error
      emit authError( this );
    }
    else
    {
      if ( !err_msg.isEmpty() )
      {
        Smb4KNotification *notification = new Smb4KNotification();
        notification->retrievingPreviewFailed( m_share, err_msg.join( "\n" ) );
      }
      else
      {
        // Do nothing
      }
    }
  }
  else
  {
    // Go ahead
  }  
}


void Smb4KPreviewJob::slotProcessFinished( int /*exitCode*/, QProcess::ExitStatus status )
{
  switch ( status )
  {
    case QProcess::CrashExit:
    {
      if ( !m_proc->isAborted() )
      {
        Smb4KNotification *notification = new Smb4KNotification();
        notification->processError( m_proc->error() );;
      }
      else
      {
        // Do nothing
      }
      break;
    }
    default:
    {
      break;
    }
  }

  emitPercent( 1, 1 );
  emitResult();
  emit finished( m_share, m_url );
}



Smb4KPreviewDialog::Smb4KPreviewDialog( Smb4KShare *share, QWidget *parent )
: KDialog( parent ), m_share( share ), m_url( share->url() ), m_iterator( QStringList() )
{
  setAttribute( Qt::WA_DeleteOnClose, true );

  setCaption( i18n( "Preview" ) );
  setButtons( Close );
  setDefaultButton( Close );

  // Set the IP address if necessary.
  if ( share->hostIP().isEmpty() )
  {
    Smb4KHost *host = findHost( share->hostName(), share->workgroupName() );
    share->setHostIP( host->ip() );
  }
  else
  {
    // Do nothing
  }

  setupView();

  connect( this,                   SIGNAL( closeClicked() ),
           this,                   SLOT( slotCloseClicked() ) );

  setMinimumWidth( sizeHint().width() > 350 ? sizeHint().width() : 350 );

  KConfigGroup group( Smb4KSettings::self()->config(), "PreviewDialog" );
  restoreDialogSize( group );

  QTimer::singleShot( 0, this, SLOT( slotRequestPreview() ) );
}


Smb4KPreviewDialog::~Smb4KPreviewDialog()
{
}


void Smb4KPreviewDialog::setupView()
{
  // Main widget
  QWidget *main_widget = new QWidget( this );
  setMainWidget( main_widget );

  QGridLayout *layout = new QGridLayout( main_widget );
  layout->setSpacing( 5 );
  layout->setMargin( 0 );

  m_view = new KListWidget( main_widget );
  m_view->setResizeMode( KListWidget::Adjust );
  m_view->setWrapping( true );
  m_view->setSortingEnabled( true );
  m_view->setWhatsThis( i18n( "The preview is displayed here." ) );
  int icon_size = KIconLoader::global()->currentSize( KIconLoader::Small );
  m_view->setIconSize( QSize( icon_size, icon_size ) );

  KToolBar *toolbar = new KToolBar( main_widget, true, false );

  m_reload  = new KAction( KIcon( "view-refresh" ), i18n( "Reload" ), toolbar );
  m_reload->setEnabled( false );
  
  m_abort   = new KAction( KIcon( "process-stop" ), i18n( "Abort" ), toolbar );
  m_abort->setEnabled( false );
  
  m_back    = new KAction( KIcon( "go-previous" ), i18n( "Back" ), toolbar );
  m_back->setEnabled( false );
  
  m_forward = new KAction( KIcon( "go-next" ), i18n( "Forward" ), toolbar );
  m_forward->setEnabled( false );
  
  m_up      = new KAction( KIcon( "go-up" ), i18n( "Up" ), toolbar );
  m_up->setEnabled( false );
  
  m_combo   = new KHistoryComboBox( true, toolbar );
  m_combo->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
  m_combo->setEditable( false );
  m_combo->setWhatsThis( i18n( "The current UNC address is shown here. You can also choose one of "
    "the previously visited locations from the drop-down menu that will then be displayed in the "
    "view above." ) );

  toolbar->addAction( m_reload );
  toolbar->addAction( m_abort );
  toolbar->addAction( m_back );
  toolbar->addAction( m_forward );
  toolbar->addAction( m_up );
  toolbar->insertSeparator( toolbar->addWidget( m_combo ) );

  layout->addWidget( m_view, 0, 0, 0 );
  layout->addWidget( toolbar, 1, 0, 0 );

  connect( toolbar, SIGNAL( actionTriggered( QAction * ) ),
           this,      SLOT( slotActionTriggered( QAction * ) ) );

  connect( m_combo,   SIGNAL( activated( const QString & ) ),
           this,      SLOT( slotItemActivated( const QString & ) ) );

  connect( m_view,    SIGNAL( executed( QListWidgetItem * ) ),
           this,      SLOT( slotItemExecuted( QListWidgetItem * ) ) );

  connect( KGlobalSettings::self(), SIGNAL( iconChanged( int ) ),
           this,                    SLOT( slotIconSizeChanged( int ) ) );
}


/////////////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATIONS
/////////////////////////////////////////////////////////////////////////////

void Smb4KPreviewDialog::slotActionTriggered( QAction *action )
{
  KAction *kaction = static_cast<KAction *>( action );

  if ( kaction )
  {
    if ( kaction == m_reload )
    {
      // Clear the history
      m_history.clear();
      
     // Request the preview
      slotRequestPreview();
    }
    else if ( kaction == m_abort )
    {
      // Emit signal to kill the preview job.
      emit abortPreview( m_share );
    }
    else if ( kaction == m_back )
    {
      // Get the current history if necessary,
      // shift one item back an request a preview.
      if ( m_history.isEmpty() )
      {
        m_history = m_combo->historyItems();
        m_iterator = QStringListIterator( m_history );
      }
      else
      {
        // Do nothing
      }

      if ( m_iterator.hasNext() )
      {
        // Jump behind current item
        (void) m_iterator.next();

        QString location = m_iterator.next();
        QString path = location.remove( m_share->unc(), Qt::CaseInsensitive );

        if ( !path.isEmpty() )
        {
          m_url.setPath( QString( "%1%2" ).arg( m_share->shareName() ).arg( path ) );
        }
        else
        {
          m_url.setPath( m_share->shareName() );
        }

        // Request the preview
        slotRequestPreview();
      }
      else
      {
        // Do nothing
      }
    }
    else if ( kaction == m_forward )
    {
      // Shift one item forward an request a preview.
      if ( !m_history.isEmpty() && m_iterator.hasPrevious() )
      {
        // Jump in front of the current item
        (void) m_iterator.previous();
        
        QString location = m_iterator.previous();
        QString path = location.remove( m_share->unc(), Qt::CaseInsensitive );

        if ( !path.isEmpty() )
        {
          m_url.setPath( QString( "%1%2" ).arg( m_share->shareName() ).arg( path ) );
        }
        else
        {
          m_url.setPath( m_share->shareName() );
        }

        // Request the preview
        slotRequestPreview();
      }
      else
      {
        // Do nothing
      }
    }
    else if ( kaction == m_up )
    {
      if ( QString::compare( m_share->unc(),
                             m_url.toString( QUrl::RemoveScheme|QUrl::RemoveUserInfo|QUrl::RemovePort ),
                             Qt::CaseInsensitive ) != 0 )
      {
        // Clear the history
        m_history.clear();

        // Adjust the path
        QString path = m_url.path();
        m_url.setPath( path.section( "/", 0, -2 ) );

        // Request the preview
        slotRequestPreview();
      }
      else
      {
        // Do nothing
      }
    }
    else
    {
      // Do nothing
    }
  }
  else
  {
    // Do nothing
  }
}


void Smb4KPreviewDialog::slotRequestPreview()
{
  // Set the current URL in the combo box
  QString current = m_url.toString( QUrl::RemoveScheme|QUrl::RemoveUserInfo|QUrl::RemovePort ).replace( m_url.host(), m_share->hostName() );

  // Set the history
  QStringList history;
  history << current;
  history << m_combo->historyItems();

  m_combo->setHistoryItems( history, true );
  m_combo->setCurrentItem( current );

  // Clear the view
  m_view->clear();
  
  // Request the preview for the current URL
  emit requestPreview( m_share, m_url, parentWidget() );
}


void Smb4KPreviewDialog::slotDisplayPreview( const QUrl &url, const QList<Item> &contents )
{
  if ( m_url != url )
  {
    return;
  }
  else
  {
    // Do nothing
  }

  // Display the preview
  for ( int i = 0; i < contents.size(); i++ )
  {
    switch ( contents.at( i ).first )
    {
      case HiddenDirectoryItem:
      {
        // Honor the user's setting about hidden items. And do not show the '.' and '..' directories.
        if ( Smb4KSettings::previewHiddenItems() &&
             QString::compare( contents.at( i ).second.value( "name" ), "." ) != 0 &&
             QString::compare( contents.at( i ).second.value( "name" ), ".." ) != 0 )
        {
          QListWidgetItem *listItem = new QListWidgetItem( KIcon( "folder" ), contents.at( i ).second.value( "name" ), m_view, Directory );
          listItem->setData( Qt::UserRole, contents.at( i ).second.value( "name" ) );
        }
        else
        {
          // Do nothing
        }
        break;
      }
      case DirectoryItem:
      {
        // Do not show the '.' and '..' directories.
        if ( QString::compare( contents.at( i ).second.value( "name" ), "." ) != 0 &&
             QString::compare( contents.at( i ).second.value( "name" ), ".." ) != 0 )
        {
          QListWidgetItem *listItem = new QListWidgetItem( KIcon( "folder" ), contents.at( i ).second.value( "name" ), m_view, Directory );
          listItem->setData( Qt::UserRole, contents.at( i ).second.value( "name" ) );
        }
        else
        {
          // Do nothing
        }
        break;
      }
      case HiddenFileItem:
      {
        if ( Smb4KSettings::previewHiddenItems() )
        {
          KUrl url( contents.at( i ).second.value( "name" ) );
          QListWidgetItem *listItem = new QListWidgetItem( KIcon( KMimeType::iconNameForUrl( url, 0 ) ), contents.at( i ).second.value( "name" ), m_view, File );
          listItem->setData( Qt::UserRole, contents.at( i ).second.value( "name" ) );
        }
        else
        {
          // Do nothing
        }
        break;
      }
      case FileItem:
      {
        KUrl url( contents.at( i ).second.value( "name" ) );
        QListWidgetItem *listItem = new QListWidgetItem( KIcon( KMimeType::iconNameForUrl( url, 0 ) ), contents.at( i ).second.value( "name" ), m_view, File );
        listItem->setData( Qt::UserRole, contents.at( i ).second.value( "name" ) );
        break;
      }
      default:
      {
        break;
      }
    }
  }

  // Enable/disable the back action.
  bool enable_back = (m_combo->historyItems().size() > 1 &&
                     (m_history.isEmpty() || m_iterator.hasNext()));
  m_back->setEnabled( enable_back );

  // Enable/disable the forward action.
  bool enable_forward = (!m_history.isEmpty() && m_iterator.hasPrevious());
  m_forward->setEnabled( enable_forward );

  // Enable/disable the up action.
  bool enable_up = (QString::compare( m_share->unc(),
                                      m_url.toString( QUrl::RemoveScheme|QUrl::RemoveUserInfo|QUrl::RemovePort ),
                                      Qt::CaseInsensitive ) != 0);
  m_up->setEnabled( enable_up );
}


void Smb4KPreviewDialog::slotAboutToStart( Smb4KShare *share, const QUrl &url )
{
  if ( share == m_share && url == m_url )
  {
    m_reload->setEnabled( false );
    m_abort->setEnabled( true );
  }
  else
  {
    // Do nothing
  }
}


void Smb4KPreviewDialog::slotFinished( Smb4KShare *share, const QUrl &url )
{
  if ( share == m_share && url == m_url )
  {
    m_reload->setEnabled( true );
    m_abort->setEnabled( false );
  }
  else
  {
    // Do nothing
  }
}


void Smb4KPreviewDialog::slotItemExecuted( QListWidgetItem *item )
{
  if ( item )
  {
    switch ( item->type() )
    {
      case Directory:
      {
        // Clear the history
        m_history.clear();
        
        if ( !Smb4KPreviewer::self()->isRunning( m_share ) )
        {
          QString old_path = m_url.path();
          m_url.setPath( QString( "%1/%2" ).arg( old_path ).arg( item->data( Qt::UserRole ).toString() ) );
          slotRequestPreview();
        }
        else
        {
          // Do nothing
        }
        break;
      }
      default:
      {
        break;
      }
    }
  }
  else
  {
    // Do nothing
  }
}


void Smb4KPreviewDialog::slotItemActivated( const QString &item )
{
  if ( !Smb4KPreviewer::self()->isRunning( m_share ) )
  {
    // Clear the history
    m_history.clear();

    m_url.setPath( QUrl( item ).path() );
    slotRequestPreview();
  }
  else
  {
    // Do nothing
  }
}


void Smb4KPreviewDialog::slotCloseClicked()
{
  KConfigGroup group( Smb4KSettings::self()->config(), "PreviewDialog" );
  saveDialogSize( group, KConfigGroup::Normal );
  emit aboutToClose( this );
}


void Smb4KPreviewDialog::slotIconSizeChanged( int group )
{
  switch ( group )
  {
    case KIconLoader::Small:
    {
      int icon_size = KIconLoader::global()->currentSize( KIconLoader::Small );
      m_view->setIconSize( QSize( icon_size, icon_size ) );
      break;
    }
    default:
    {
      break;
    }
  }
}


Smb4KPreviewerPrivate::Smb4KPreviewerPrivate()
{
}


Smb4KPreviewerPrivate::~Smb4KPreviewerPrivate()
{
}

#include "smb4kpreviewer_p.moc"

