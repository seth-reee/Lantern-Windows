import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore

ApplicationWindow {
 id: root
 width: 1120; height: 740; minimumWidth: 980; minimumHeight: 620
 visible: true; title: "Lantern"
 color: theme.colors.background
 font.pixelSize: 13
 palette.window: theme.colors.background
 palette.base: theme.colors.lighter_background
 palette.button: theme.colors.lighter_background
 palette.text: theme.colors.foreground
 palette.windowText: theme.colors.foreground
 palette.buttonText: theme.colors.foreground
 palette.highlight: theme.colors.accent
 palette.highlightedText: theme.colors.highlighted_text
 palette.placeholderText: theme.colors.muted
 property string licenseText: "MIT License\n\nCopyright (c) 2026 seth-reee\n\nPermission is hereby granted, free of charge, to any person obtaining a copy\nof this software and associated documentation files (the \"Software\"), to deal\nin the Software without restriction, including without limitation the rights\nto use, copy, modify, merge, publish, distribute, sublicense, and/or sell\ncopies of the Software, and to permit persons to whom the Software is\nfurnished to do so, subject to the following conditions:\n\nThe above copyright notice and this permission notice shall be included in all\ncopies or substantial portions of the Software.\n\nTHE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\nIMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\nFITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\nAUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\nLIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\nOUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\nSOFTWARE.\n"
 property string selectedIp: ""
 property string selectedInfo: ""
 property int sortColumn: 0
 property bool descending: false
 function applyFilter() { if (search && statusFilter && hideDead) hosts.filter(search.text, statusFilter.currentIndex === 0 ? "" : statusFilter.currentText, hideDead.checked) }
 component ActionButton: Button {
  id: button
  padding: 10; leftPadding: 15; rightPadding: 15
  background: Rectangle { radius: 6; color: button.down ? theme.colors.selection : theme.colors.lighter_background; border.color: button.hovered || button.activeFocus ? theme.colors.accent : theme.colors.muted; opacity: button.enabled ? 1 : 0.4 }
 }
 component HostMenuItem: MenuItem {
  id: action
  implicitWidth: 210; implicitHeight: 36
  leftPadding: 12; rightPadding: 12
  contentItem: Label {
   text: action.text; font: action.font
   color: action.enabled ? theme.colors.foreground : theme.colors.light_foreground
   verticalAlignment: Text.AlignVCenter
  }
  background: Rectangle {
   radius: 4
   color: action.highlighted ? theme.colors.selection : "transparent"
  }
 }
 component EntryField: TextField {
  id: field
  padding: 10; selectByMouse: true
  background: Rectangle { radius: 6; color: theme.colors.lighter_background; border.color: field.activeFocus ? theme.colors.accent : theme.colors.muted }
 }
 component Stat: Rectangle {
  property string caption
  property string value
  property color ink: theme.colors.foreground
  Layout.fillWidth: true; implicitHeight: 68; radius: 6
  color: theme.colors.lighter_background
  Column { anchors.left: parent.left; anchors.leftMargin: 14; anchors.verticalCenter: parent.verticalCenter; spacing: 3
   Label { text: value; color: ink; font.pixelSize: 23; font.bold: true }
   Label { text: caption; color: theme.colors.foreground; opacity: 0.65; font.pixelSize: 11 }
  }
 }
 ColumnLayout {
  anchors.fill: parent; anchors.margins: 20; spacing: 12
  RowLayout {
   Image { source: theme.iconSource; sourceSize.width: 84; sourceSize.height: 84; Layout.preferredWidth: 42; Layout.preferredHeight: 42; fillMode: Image.PreserveAspectFit }
   ColumnLayout { spacing: 2
    Label { text: "Lantern"; font.pixelSize: 24; font.bold: true }
    Label { text: "Network IP scanner"; opacity: 0.65 }
   }
   Item { Layout.fillWidth: true }
   ActionButton { text: "About"; onClicked: about.open() }
  }
  Rectangle {
   Layout.fillWidth: true; implicitHeight: scanControls.implicitHeight+28
   color: "transparent"; radius: 6; border.color: theme.colors.muted
   ColumnLayout { id: scanControls; anchors.fill: parent; anchors.margins: 14; spacing: 10
    RowLayout {
     Label { text: "Scan range"; font.bold: true }
     Item { Layout.fillWidth: true }
     Label { text: "IPv4 · CIDR (up to /16), range, or single address"; opacity: 0.55; font.pixelSize: 11 }
    }
    RowLayout {
     EntryField { id: range; objectName: "rangeField"; text: scanner.localRange; placeholderText: "192.168.1.0/24"; Layout.fillWidth: true; enabled: !scanner.running; onAccepted: startButton.clicked() }
     ActionButton { text: "Local /24"; enabled: !scanner.running; onClicked: range.text=scanner.localRange }
     ActionButton { id: startButton; objectName: "startButton"; text: scanner.running ? "Scanning…" : "Start scan"; enabled: !scanner.running; onClicked: { root.selectedIp=""; root.selectedInfo=""; scanner.start(range.text, dns.checked, ports.checked, timeout.value, parallel.value) } }
     ActionButton { text: "Stop"; enabled: scanner.running; onClicked: scanner.stop() }
    }
    RowLayout {
     CheckBox { id: dns; text: "Resolve hostnames"; checked: true; enabled: !scanner.running }
     CheckBox { id: ports; text: "Common ports"; checked: true; enabled: !scanner.running; ToolTip.visible: hovered; ToolTip.text: "23 TCP ports: FTP, SSH, Telnet, mail, DNS, web, SMB, print, databases, RDP, VNC" }
     Item { Layout.fillWidth: true }
     Label { text: "Timeout (ms)"; opacity: 0.65 }
     SpinBox { id: timeout; from: 200; to: 5000; stepSize: 100; value: 800; editable: true; enabled: !scanner.running; Layout.preferredWidth: 115 }
     Label { text: "Parallel hosts"; opacity: 0.65 }
     SpinBox { id: parallel; from: 1; to: 64; value: 32; editable: true; enabled: !scanner.running; Layout.preferredWidth: 120 }
    }
   }
  }
  RowLayout { spacing: 10
   Stat { caption: "ADDRESSES"; value: scanner.total.toString() }
   Stat { caption: "LIVE"; value: scanner.alive.toString(); ink: theme.colors.green }
   Stat { caption: "NO REPLY"; value: scanner.silent.toString(); ink: theme.colors.red }
   Stat { caption: "COMPLETED"; value: scanner.completed + " / " + scanner.total }
  }
  RowLayout {
   EntryField { id: search; placeholderText: "Filter by IP, hostname, MAC, or port…"; Layout.fillWidth: true; onTextChanged: root.applyFilter() }
   CheckBox { id: hideDead; text: "Hide dead IPs"; onCheckedChanged: root.applyFilter(); ToolTip.visible: hovered; ToolTip.text: "Hide hosts with No reply status" }
   ComboBox { id: statusFilter; model: ["All hosts", "Live", "No reply", "Error", "Cancelled"]; onCurrentIndexChanged: root.applyFilter(); Layout.preferredWidth: 140 }
   ActionButton { text: "Export CSV"; enabled: scanner.total > 0 && !scanner.running; onClicked: exportDialog.open() }
  }
  Rectangle {
   Layout.fillWidth: true; Layout.fillHeight: true; radius: 6; color: "transparent"; border.color: theme.colors.muted; clip: true
   ColumnLayout { anchors.fill: parent; anchors.margins: 1; spacing: 0
    Rectangle { Layout.fillWidth: true; height: 37; color: theme.colors.lighter_background
     RowLayout { anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12; spacing: 12
      Repeater { model: ["IP address", "Status", "Hostname", "Ping", "Open TCP ports", "MAC address"]
       delegate: Label { required property string modelData; required property int index
        text: modelData + (root.sortColumn === index ? (root.descending ? " ↓" : " ↑") : ""); font.bold: true
        Layout.preferredWidth: index===0 ? 140 : index===1 ? 95 : index===3 ? 85 : index===5 ? 155 : 0
        Layout.fillWidth: index===2 || index===4
        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { root.descending=root.sortColumn===index ? !root.descending : false; root.sortColumn=index; hosts.order(index,root.descending) } }
       }
      }
     }
    }
    ListView {
     id: results; objectName: "results"; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: hosts
     ScrollBar.vertical: ScrollBar {}
     delegate: Rectangle {
      id: row
      required property string ip; required property string status; required property string hostname
      required property string mac; required property string latency; required property string ports; required property string detail; required property int index
      width: results.width; height: 40
      color: root.selectedIp===ip ? theme.colors.selection : (index%2 ? Qt.rgba(0.5,0.5,0.5,0.035) : "transparent")
      RowLayout { anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12; spacing: 12
       Label { text: row.ip; Layout.preferredWidth: 140; font.family: "monospace" }
       Label { text: row.status; Layout.preferredWidth: 95; color: row.status==="Live" ? theme.colors.green : row.status==="No reply" || row.status==="Error" ? theme.colors.red : theme.colors.foreground; opacity: row.status==="Queued" ? 0.4 : 1 }
       Label { text: row.hostname || "—"; Layout.fillWidth: true; Layout.preferredWidth: 0; elide: Text.ElideRight }
       Label { text: row.latency || "—"; Layout.preferredWidth: 85 }
       Label { text: row.ports || "—"; Layout.fillWidth: true; Layout.preferredWidth: 0; elide: Text.ElideRight }
       Label { text: row.mac || "—"; Layout.preferredWidth: 155; font.family: "monospace" }
      }
      MouseArea { anchors.fill: parent; acceptedButtons: Qt.LeftButton | Qt.RightButton
       onClicked: mouse => { root.selectedIp=row.ip; root.selectedInfo=[row.ip,row.status,row.hostname,row.mac,row.latency,row.ports,row.detail].filter(s=>s.length).join(" · "); if(mouse.button===Qt.RightButton) rowMenu.popup() }
       onDoubleClicked: scanner.copy(row.ip)
      }
      Menu {
       id: rowMenu; objectName: "hostContextMenu"
       implicitWidth: 220; padding: 5
       background: Rectangle { radius: 6; color: theme.colors.lighter_background; border.color: theme.colors.muted }
       HostMenuItem { objectName: "copyIpAction"; text: "Copy IP address"; onTriggered: scanner.copy(row.ip) }
       HostMenuItem { text: "Copy host details"; onTriggered: scanner.copy(root.selectedInfo) }
      }
     }
     Label { anchors.centerIn: parent; visible: results.count===0; text: scanner.total===0 ? "Choose a range and start a scan.\nHosts will appear here as they are checked." : "No hosts match your filters."; horizontalAlignment: Text.AlignHCenter; opacity: 0.5; lineHeight: 1.5 }
    }
   }
  }
  Label { text: root.selectedInfo || "Select a host for details · Double-click to copy its IP"; Layout.fillWidth: true; elide: Text.ElideRight; opacity: 0.65; ToolTip.visible: detailsHover.hovered; ToolTip.text: text; HoverHandler { id: detailsHover } }
  ProgressBar { Layout.fillWidth: true; from: 0; to: Math.max(1,scanner.total); value: scanner.completed; implicitHeight: 4 }
  RowLayout { Label { objectName: "scanTimer"; text: "Scanned in: " + (scanner.elapsedMs / 1000).toFixed(1) + " s" } Label { text: scanner.message || "Ready"; Layout.fillWidth: true; elide: Text.ElideRight } Label { text: "No reply can mean ping is blocked"; opacity: 0.5; font.pixelSize: 11 } }
 }
 FileDialog { id: exportDialog; objectName: "exportDialog"; title: "Export scan results"; fileMode: FileDialog.SaveFile; defaultSuffix: "csv"; nameFilters: ["CSV files (*.csv)"]; currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation); onAccepted: scanner.exportCsv(selectedFile) }
 Dialog { id: about; objectName: "aboutDialog"; title: "About Lantern"; anchors.centerIn: parent; modal: true; standardButtons: Dialog.Close; width: 580
  contentItem: ColumnLayout { spacing: 10
   Image { source: theme.iconSource; sourceSize.width: 72; sourceSize.height: 72; Layout.preferredWidth: 72; Layout.preferredHeight: 72 }
   Label { text: "Lantern " + Qt.application.version; font.pixelSize: 23; font.bold: true }
   Label { text: "A lightweight network IP scanner for Windows." }
   Label { text: 'Created by seth-reee · <a href="https://github.com/seth-reee">GitHub profile</a> · <a href="https://github.com/seth-reee/Lantern">Source code</a>'; textFormat: Text.RichText; onLinkActivated: link => Qt.openUrlExternally(link); palette.link: theme.colors.accent }
   Label { text: "MIT License · Copyright © 2026 seth-reee" }
   ScrollView { Layout.fillWidth: true; Layout.preferredHeight: 190; clip: true
    TextArea { readOnly: true; selectByMouse: true; wrapMode: TextEdit.Wrap; text: licenseText; font.pixelSize: 12 }
   }
  }
 }
 Shortcut { sequence: "Ctrl+F"; onActivated: search.forceActiveFocus() }
 Shortcut { sequence: "Escape"; onActivated: { if(scanner.running) scanner.stop() } }
}
