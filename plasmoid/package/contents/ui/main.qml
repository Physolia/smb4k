/***************************************************************************
    main.qml - The main file for Smb4K's plasmoid
                             -------------------
    begin                : Sa Feb 11 2012
    copyright            : (C) 2012 by Alexander Reinholdt
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

import QtQuick 1.1
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.graphicswidgets 0.1 as PlasmaWidgets
import org.kde.plasma.components 0.1 as PlasmaComponents
// import org.kde.plasma.graphicslayouts 4.7 as GraphicsLayouts
import org.kde.qtextracomponents 0.1


Item {
  id: mainwindow
  property int minimumWidth: 300
  property int minimumHeight: 200
  property string parent_item: ""
  property int parent_type: 0 // Unknown
  property url parent_url
  
  Scanner {
    id: scanner
    onWorkgroupsChanged: {
      getWorkgroups()
    }
    onHostsChanged: {
      getHosts()
    }
    onSharesChanged: {
      getShares()
    }
    onAboutToStart: {
      busy()
    }
    onFinished: {
      idle()
    }
  }

  Mounter {
    id: mounter
    onMounted: {
      addMountedShares()
    }
    onUnmounted: {
      removeUnmountedShares()
    }
  }

  //
  // Delegate for the list items in the browser
  //
  Component {
    id: browserItemDelegate
    PlasmaComponents.ListItem {
      width: browserListView.width
      height: 40
      Row {
        spacing: 5
        Column {
          anchors.verticalCenter: parent.verticalCenter
          QIconItem {
            icon: itemIcon
            width: 22
            height: 22
          }
        }
        Column {
          anchors.verticalCenter: parent.verticalCenter
          Text { text: itemName }
          Text { text: "<font size=\"-1\">"+itemComment+"</font>" }
        }
      }
      MouseArea {
        anchors.fill: parent
        onClicked: {
          browserListView.currentIndex = index
          networkItemClicked()
        }
      }
    }
  }

  //
  // Delegate for the items in the shares view
  //
  Component {
    id: sharesViewItemDelegate
    Item {
      width: sharesView.cellWidth
      height: sharesView.cellHeight
      Column {
        Row {
          QIconItem {
            icon: itemIcon
            width: 32
            height: 32
            anchors.horizontalCenter: parent.horizontalCenter
          }
        }
        Text {
          text: itemName
          anchors.horizontalCenter: parent.horizontalCenter
        }
        Text {
          text: i18n( "<font size=\"-1\">on %1</font>" ).arg( itemHost )
          anchors.horizontalCenter: parent.horizontalCenter
        }
      }
      MouseArea {
        anchors.fill: parent
        onClicked: {
          sharesView.currentIndex = index
          mountedShareClicked()
        }
      }
    }
  }

  //
  // The tool bar
  //
  PlasmaComponents.ToolBar {
    id: toolBar
    clip: true
    anchors {
      top: parent.top
      left: parent.left
      right: parent.right
    }

    PlasmaComponents.ToolBarLayout {
      id: toolBarLayout
      
      PlasmaComponents.ToolButton {
        id: rescanButton
        iconSource: "view-refresh"
        enabled: true
        onClicked: {
          rescan()
        }
      }
      
      PlasmaComponents.ToolButton {
        id: abortButton
        iconSource: "process-stop"
        enabled: false
        onClicked: {
          abort()
        }
      }
        
      PlasmaComponents.ToolButton {
        id: upButton
        iconSource: "go-up"
        enabled: false
        onClicked: {
          up()
        }
      }
        
      PlasmaComponents.BusyIndicator {
        id: busyIndicator
        height: 22
        width: 22
        visible: false
      }
    }
      
    tools: toolBarLayout
    transition: "set"
  }

  //
  // The browser widget
  //
  // FIXME: With KDE SC 4.9 move to PlasmaComponents.ScrollArea
  ListView {
    id: browserListView
    anchors {
      top: toolBar.bottom
      left: parent.left
      bottom: parent.bottom
    }
    anchors.topMargin: 2
    anchors.rightMargin: 5
    width: parent.width / 2
    delegate: browserItemDelegate
    model: ListModel {}
    focus: true
    highlightFollowsCurrentItem: true
    highlightRangeMode: ListView.StrictlyEnforceRange
    highlight: PlasmaComponents.Highlight

    PlasmaComponents.ScrollBar {
      flickableItem: parent
      anchors {
        right: parent.right
        top: parent.top
        bottom: parent.bottom
      }
    }
  }

  //
  // The mounted shares view
  //
  GridView {
    id: sharesView
    cellWidth: 120
    cellHeight: 80
    anchors {
      top: toolBar.bottom
      left: browserListView.right
      right: parent.right
      bottom: parent.bottom
    }
    anchors.topMargin: 5
    anchors.leftMargin: 2
    width: parent.width / 2
    delegate: sharesViewItemDelegate
    model: ListModel {}
    focus: true
    highlightFollowsCurrentItem: true
    highlightRangeMode: GridView.StrictlyEnforceRange
    highlight: PlasmaComponents.Highlight

    PlasmaComponents.ScrollBar {
      flickableItem: parent
      anchors {
        right: parent.right
        top: parent.top
        bottom: parent.bottom
      }
    }
  }

  Component.onCompleted: {
    scanner.start()
    mounter.start()
  }

  //
  // Get the workgroups and show them in the list view
  //
  function getWorkgroups() {
    
    if ( parent_type != 0 /* unknown aka entire network */ ) {
      return
    }
    else {
      upButton.enabled = false
    }

    // Remove obsolete workgroups
    if ( browserListView.model.count != 0 ) {
      obsolete_items = new Array()
      
      for ( var i = 0; i < browserListView.model.count; i++ ) {
        var object = scanner.find( browserListView.model.get( i ).itemURL, browserListView.model.get( i ).itemType )
        
        if ( !object ) {
          obsolete_items.push( browserListView.model.get( i ).itemName )
        }
        else {
          // Do nothing
        }
      }
      
      if ( obsolete_items.length != 0 ) {
        for ( var i = 0; i < obsolete_items.length; i++ ) {
          for ( var j = 0; j < browserListView.model.count; j++ ) {
            if ( obsolete_items[i] == browserListView.model.get( j ).itemName ) {
              browserListView.model.remove( j )
              break
            }
            else {
              continue
            }
          }
        }
      }
      else {
        // Do nothing
      }
    }
    else {
      // Do nothing
    }
    
    // Add new workgroups
    if ( scanner.workgroups.length != 0 ) {
      for ( var i = 0; i < scanner.workgroups.length; i++ ) {
        var have_item = false
        
        if ( browserListView.model.count != 0 ) {
          for ( var j = 0; j < browserListView.model.count; j++ ) {
            if ( browserListView.model.get( j ).itemName == scanner.workgroups[i].workgroupName ) {
              have_item = true
              break
            }
            else {
              // Do nothing
            }
          }
        }
        
        if ( !have_item ) {
          browserListView.model.append( { 
                                 "itemName": scanner.workgroups[i].workgroupName,
                                 "itemComment": scanner.workgroups[i].comment,
                                 "itemIcon": scanner.workgroups[i].icon, 
                                 "itemURL": scanner.workgroups[i].url,
                                 "itemType": scanner.workgroups[i].type,
                                 "itemIsMounted": scanner.workgroups[i].isMounted } )
        }
        else {
          // Do nothing
        }
      }
    }
    else {
      while ( browserListView.model.count != 0 ) {
        browserListView.model.remove( 0 )
      }
    }
  }

  //
  // Get the hosts and show them in the list view
  //
  function getHosts() {
    
    if ( parent_type != 1 /* workgroup */ ) {
      return
    }
    else {
      upButton.enabled = true
    }
    
    // Remove obsolete hosts
    if ( browserListView.model.count != 0 ) {
      obsolete_items = new Array()
      
      for ( var i = 0; i < browserListView.model.count; i++ ) {
        var object = scanner.find( browserListView.model.get( i ).itemURL, browserListView.model.get( i ).itemType )
        
        if ( !object ) {
          obsolete_items.push( browserListView.model.get( i ).itemName )
        }
        else {
          // Do nothing
        }
      }
      
      if ( obsolete_items.length != 0 ) {
        for ( var i = 0; i < obsolete_items.length; i++ ) {
          for ( var j = 0; j < browserListView.model.count; j++ ) {
            if ( obsolete_items[i] == browserListView.model.get( j ).itemName ) {
              browserListView.model.remove( j )
              break
            }
            else {
              continue
            }
          }
        }
      }
      else {
        // Do nothing
      }
    }
    else {
      // Do nothing
    }
    
    // Add new hosts
    if ( scanner.hosts.length != 0 ) {
      for ( var i = 0; i < scanner.hosts.length; i++ ) {
        
        if ( scanner.hosts[i].workgroupName != parent_item ) {
          continue
        }
        else {
          // Do nothing
        }
        
        var have_item = false
        
        if ( browserListView.model.count != 0 ) {
          for ( var j = 0; j < browserListView.model.count; j++ ) {
            if ( browserListView.model.get( j ).itemName == scanner.hosts[i].hostName ) {
              have_item = true
              break
            }
            else {
              // Do nothing
            }
          }
        }
        
        if ( !have_item ) {
          browserListView.model.append( { 
                                 "itemName": scanner.hosts[i].hostName,
                                 "itemComment": scanner.hosts[i].comment,
                                 "itemIcon": scanner.hosts[i].icon, 
                                 "itemURL": scanner.hosts[i].url,
                                 "itemType": scanner.hosts[i].type,
                                 "itemIsMounted": scanner.hosts[i].isMounted } )
        }
        else {
          // Do nothing
        }
      }
    }
    else {
      while ( browserListView.model.count != 0 ) {
        browserListView.model.remove( 0 )
      }
    }
  }

  //
  // Get the shares and show them in the list view
  //
  function getShares() {
    
    if ( parent_type != 2 /* host */ ) {
      return
    }
    else {
      upButton.enabled = true
    }
    
    // Remove obsolete shares
    if ( browserListView.model.count != 0 ) {
      obsolete_items = new Array()
      
      for ( var i = 0; i < browserListView.model.count; i++ ) {
        var object = scanner.find( browserListView.model.get( i ).itemURL, browserListView.model.get( i ).itemType )
        
        if ( !object ) {
          obsolete_items.push( browserListView.model.get( i ).itemName )
        }
        else {
          // Do nothing
        }
      }
      
      if ( obsolete_items.length != 0 ) {
        for ( var i = 0; i < obsolete_items.length; i++ ) {
          for ( var j = 0; j < browserListView.model.count; j++ ) {
            if ( obsolete_items[i] == browserListView.model.get( j ).itemName ) {
              browserListView.model.remove( j )
              break
            }
            else {
              continue
            }
          }
        }
      }
      else {
        // Do nothing
      }
    }
    else {
      // Do nothing
    }
    
    // Add new shares
    if ( scanner.shares.length != 0 ) {
      for ( var i = 0; i < scanner.shares.length; i++ ) {
        
        if ( scanner.shares[i].hostName != parent_item ) {
          continue
        }
        else {
          // Do nothing
        }
        
        var have_item = false
        
        if ( browserListView.model.count != 0 ) {
          for ( var j = 0; j < browserListView.model.count; j++ ) {
            if ( browserListView.model.get( j ).itemName == scanner.shares[i].shareName ) {
              have_item = true
              break
            }
            else {
              // Do nothing
            }
          }
        }
        
        if ( !have_item ) {
          browserListView.model.append( { 
                                 "itemName": scanner.shares[i].shareName,
                                 "itemComment": scanner.shares[i].comment,
                                 "itemIcon": scanner.shares[i].icon, 
                                 "itemURL": scanner.shares[i].url,
                                 "itemType": scanner.shares[i].type,
                                 "itemIsMounted": scanner.shares[i].isMounted } )
        }
        else {
          // Do nothing
        }
      }
    }
    else {
      while ( browserListView.model.count != 0 ) {
        browserListView.model.remove( 0 )
      }
    }
  }
  
  //
  // Add mounted shares to the shares view
  //
  function addMountedShares() {
    // We know that at least one share is mounted, so we do not need 
    // to check that the list of mounted shares is empty.
    for ( var i = 0; i < mounter.mountedShares.length; i++ ) {

      var have_item = false

      if ( sharesView.model.count != 0 ) {
        for ( var j = 0; j < sharesView.model.count; j++ ) {
          if ( sharesView.model.get( j ).itemURL == mounter.mountedShares[i].url ) {
            have_item = true
            break
          }
          else {
            // Do nothing
          }
        }
      }

      if ( !have_item ) {
        sharesView.model.append( {
                          "itemName": mounter.mountedShares[i].shareName,
                          "itemHost": mounter.mountedShares[i].hostName,
                          "itemIcon": mounter.mountedShares[i].icon,
                          "itemURL": mounter.mountedShares[i].url } )
      }
      else {
        // Do nothing
      }
    }
    
    // Now modify the icon in the browser
    if ( parent_type == 2 ) {
      for ( var i = 0; i < browserListView.model.count; i++ ) {
        var object = mounter.find( browserListView.model.get( i ).itemURL, false )
        
        if ( object && parent_item == object.hostName ) {
          browserListView.model.get( i ).itemIcon = object.icon
        }
        else {
          // Do nothing
        }
      }
    }
    else {
      // Do nothing
    }
  }
  
  //
  // Remove unmounted shares from the shares view
  //
  function removeUnmountedShares() {
    // Remove obsolete shares.
    if ( sharesView.model.count != 0 ) {
      obsolete_shares = new Array()

      for ( var i = 0; i < sharesView.model.count; i++ ) {
        var object = mounter.find( sharesView.model.get( i ).itemURL )
        
        if ( !object || !object.isMounted ) {
          obsolete_shares.push( sharesView.model.get( i ).itemURL.toString() )
        }
        else {
          // Do nothing
        }
      }

      if ( obsolete_shares.length != 0 ) {
        for ( var i = 0; i < obsolete_shares.length; i++ ) {
          for ( var j = 0; j < sharesView.model.count; j++ ) {
            if ( obsolete_shares[i] == sharesView.model.get( j ).itemURL.toString() ) {
              sharesView.model.remove( j )
              break
            }
            else {
              continue
            }
          }
        }
      }
      else {
        // Do nothing
      }
    }
    else {
      // Do nothing
    }
    
    // Now modify the icon in the browser
    if ( parent_type == 2 ) {
      for ( var i = 0; i < browserListView.model.count; i++ ) {
        var object = mounter.find( browserListView.model.get( i ).itemURL, false )
        
        if ( object ) {
          if ( !object.isMounted && parent_item == object.hostName ) {
            browserListView.model.get( i ).itemIcon = object.icon
          }
          else {
            // Do nothing
          }
        }
        else {
          var obj = scanner.find( browserListView.model.get( i ).itemURL, browserListView.model.get( i ).itemType )
          
          if ( obj ) {
            browserListView.model.get( i ).itemIcon = obj.icon
          }
          else {
            // Do nothing
          }
        }
      }
    }
    else {
      // Do nothing
    }
  }
  
  //
  // An network item was clicked
  //
  function networkItemClicked() {
    
    if ( browserListView.model.get( browserListView.currentIndex ).itemType < 3 /* 3 == share */ ) {
      parent_url = browserListView.model.get( browserListView.currentIndex ).itemURL
      parent_item = browserListView.model.get( browserListView.currentIndex ).itemName
      parent_type = browserListView.model.get( browserListView.currentIndex ).itemType
      
      scanner.lookup( parent_url, parent_type )
      
      while ( browserListView.model.count != 0 ) {
        browserListView.model.remove( 0 )
      }
    }
    else {
      mounter.mount( browserListView.model.get( browserListView.currentIndex ).itemURL )
    }
  }

  //
  // A mounted share was clicked
  //
  function mountedShareClicked() {
    print( "Mounted share clicked" )
  }
  
  //
  // Rescan the network neighborhood
  //
  function rescan() {
    if ( parent_type < 3 /* 3 == share */ ) {
      scanner.lookup( parent_url, parent_type )
    }
    else {
      // Do nothing
    }   
  }
  
  //
  // Abort any actions performed by the core
  //
  function abort() {
    scanner.abortAll
    mounter.abortAll
  }
  
  //
  // Go one level up
  //
  function up() {
    switch ( parent_type )
    {
      case 1: {
        parent_url = "smb://"
        parent_item = ""
        parent_type--
        rescan()
        break
      }
      case 2: {
        var object = scanner.find( parent_url, parent_type )
        parent_url = "smb://"+object.workgroupName
        parent_item = object.workgroupName
        parent_type--
        rescan()
        break
      }
      default: {
        break
      }
    }
 
    while ( browserListView.model.count != 0 ) {
      browserListView.model.remove( 0 )
    }    
  }
  
  //
  // The application is busy
  //
  function busy() {
    if ( scanner.running ) {
      rescanButton.enabled = false
    }
    else {
      // Do nothing
    }
    abortButton.enabled = true
      
    busyIndicator.visible = true
    busyIndicator.running = true
  }
  
  //
  // The application has become idle
  //
  function idle() {
    rescanButton.enabled = true
    
    if ( !scanner.running && !mounter.running ) {
      abortButton.enabled = false
    }
    else {
      // Do nothing
    }
    
    busyIndicator.running = false
    busyIndicator.visible = false
  }
}

