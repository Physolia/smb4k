/***************************************************************************
    Smb4K's container class for information about a host.
                             -------------------
    begin                : Sa Jan 26 2008
    copyright            : (C) 2008-2017 by Alexander Reinholdt
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// application specific includes
#include "smb4khost.h"
#include "smb4kauthinfo.h"

// Qt includes
#include <QStringList>
#include <QDebug>
#include <QUrl>

// KDE includes
#include <KIconThemes/KIconLoader>


class Smb4KHostPrivate
{
  public:
    QString workgroup;
    QHostAddress ip;
    QString comment;
    bool isMaster;
};


Smb4KHost::Smb4KHost(const QString &name)
: Smb4KBasicNetworkItem(Host), d(new Smb4KHostPrivate)
{
  d->isMaster     = false;
  setHostName(name);
  setIcon(KDE::icon("network-server"));
}


Smb4KHost::Smb4KHost(const Smb4KHost &h)
: Smb4KBasicNetworkItem(Host), d(new Smb4KHostPrivate)
{
  *d = *h.d;
  
  if (icon().isNull())
  {
    setIcon(KDE::icon("network-server"));
  }
  else
  {
    // Do nothing
  }
  
  if (url().isEmpty())
  {
    qWarning() << "URL of the share item is empty";
  }
  else
  {
    // Do nothing
  }
}


Smb4KHost::Smb4KHost()
: Smb4KBasicNetworkItem(Host), d(new Smb4KHostPrivate)
{
  d->isMaster     = false;
  setIcon(KDE::icon("network-server"));
}


Smb4KHost::~Smb4KHost()
{
}


void Smb4KHost::setHostName(const QString &name)
{
  QUrl u = url();
  u.setHost(name);
  u.setScheme("smb");
  setUrl(u);
}


QString Smb4KHost::hostName() const
{
  return url().host().toUpper();
}


QString Smb4KHost::unc() const
{
  QString unc;
  
  if (!hostName().isEmpty())
  {
    unc = QString("//%1").arg(hostName());
  }
  else
  {
    // Do nothing
  }
  
  return unc;
}


void Smb4KHost::setWorkgroupName(const QString &workgroup)
{
  d->workgroup = workgroup.toUpper();
}


QString Smb4KHost::workgroupName() const
{
  return d->workgroup;
}


void Smb4KHost::setIpAddress(const QString &ip)
{
  d->ip.setAddress(ip);
}


void Smb4KHost::setIpAddress(const QHostAddress& address)
{
  if (!address.isNull() && address.protocol() != QAbstractSocket::UnknownNetworkLayerProtocol)
  {
    d->ip = address;
  }
  else
  {
    // Do nothing
  }
}


QString Smb4KHost::ipAddress() const
{
  return d->ip.toString();
}


bool Smb4KHost::hasIpAddress() const
{
  return !d->ip.isNull();
}


void Smb4KHost::setComment(const QString &comment)
{
  d->comment = comment;
}


QString Smb4KHost::comment() const
{
  return d->comment;
}


void Smb4KHost::setIsMasterBrowser(bool master)
{
  d->isMaster = master;
}


bool Smb4KHost::isMasterBrowser() const
{
  return d->isMaster;
}


bool Smb4KHost::isEmpty() const
{
  if (!url().isEmpty())
  {
    return false;
  }
  else
  {
    // Do nothing
  }

  if (!d->workgroup.isEmpty())
  {
    return false;
  }
  else
  {
    // Do nothing
  }

  if (!d->ip.isNull())
  {
    return false;
  }
  else
  {
    // Do nothing
  }

  if (!d->comment.isEmpty())
  {
    return false;
  }
  else
  {
    // Do nothing
  }

  // Do not include icon here.

  return true;
}


void Smb4KHost::setLogin(const QString &login)
{
  QUrl u = url();
  u.setUserName(login);
  setUrl(u);
}


QString Smb4KHost::login() const
{
  return url().userName();
}


void Smb4KHost::setPassword(const QString &passwd)
{
  QUrl u = url();
  u.setPassword(passwd);
  setUrl(u);
}


QString Smb4KHost::password() const
{
  return url().password();
}


void Smb4KHost::setPort(int port)
{
  QUrl u = url();
  u.setPort(port);
  setUrl(u);
}


int Smb4KHost::port() const
{
  return url().port();
}


bool Smb4KHost::equals(Smb4KHost *host) const
{
  Q_ASSERT(host);

  if (url() != host->url())
  {
    return false;
  }
  else
  {
    // Do nothing
  }

  if (QString::compare(workgroupName(), host->workgroupName()) != 0)
  {
    return false;
  }
  else
  {
    // Do nothing
  }

  if (QString::compare(ipAddress(), host->ipAddress()) != 0)
  {
    return false;
  }
  else
  {
    // Do nothing
  }

  if (QString::compare(comment(), host->comment()) != 0)
  {
    return false;
  }
  else
  {
    // Do nothing
  }

  // Do not include icon here.

  return true;
}


void Smb4KHost::setAuthInfo(Smb4KAuthInfo *authInfo)
{
  QUrl u = url();
  u.setUserName(authInfo->userName());
  u.setPassword(authInfo->password());
  setUrl(u);
}


void Smb4KHost::update(Smb4KHost* host)
{
  if (QString::compare(workgroupName(), host->workgroupName()) == 0 &&
      QString::compare(hostName(), host->hostName()) == 0)
  {
    setUrl(host->url());
    setComment(host->comment());
    setIsMasterBrowser(host->isMasterBrowser());
    
    // Do not kill the already discovered IP address
    if (!hasIpAddress() && host->hasIpAddress())
    {
      setIpAddress(host->ipAddress());
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

