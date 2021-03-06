// albert - a simple application launcher for linux
// Copyright (C) 2014-2015 Manuel Schneider
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include "main.h"
#include "query.h"
#include "xdgiconlookup.h"
#include "standarditem.h"
#include "standardaction.h"
using Core::Action;
using Core::StandardAction;
using Core::StandardItem;

/** ***************************************************************************/
VirtualBox::Extension::Extension()
    : Core::Extension("org.albert.extension.virtualbox"),
      Core::QueryHandler(Core::Extension::id) {
    QString iconPath = XdgIconLookup::instance()->themeIconPath("virtualbox");
    iconPath_ = iconPath.isNull() ? ":vbox" : iconPath;
}



/** ***************************************************************************/
QWidget *VirtualBox::Extension::widget(QWidget *parent) {
    return new QWidget(parent);
}



/** ***************************************************************************/
void VirtualBox::Extension::setupSession() {
    names_.clear();
    uuids_.clear();
    QProcess *process = new QProcess;
    process->setReadChannel(QProcess::StandardOutput);
    connect(process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
        [this, process](int exitCode, QProcess::ExitStatus exitStatus){
        if (exitStatus == QProcess::NormalExit && exitCode == 0){
            while (process->canReadLine()) {
               QString line = QString::fromLocal8Bit(process->readLine());
               QRegularExpression regex("\"(.*)\" {(.*)}");
               QRegularExpressionMatch match = regex.match(line);
               names_.push_back(match.captured(1));
               uuids_.push_back(match.captured(2));
            }
        }
        process->deleteLater();
    });
    process->start("VBoxManage",  {"list", "vms"});
}



/** ***************************************************************************/
void VirtualBox::Extension::handleQuery(Core::Query * query) {

   for (uint i = 0; i < names_.size(); ++i){
       if (names_[i].startsWith(query->searchTerm(), Qt::CaseInsensitive)) {

           std::shared_ptr<StandardItem> item = std::make_shared<StandardItem>(uuids_[i]);
           item->setText(names_[i]);
           item->setSubtext(QString("'%1' aka '%2'").arg(names_[i], uuids_[i]));
           item->setIconPath(iconPath_);

           std::shared_ptr<StandardAction> action = std::make_shared<StandardAction>();
           action->setText("Start virtual machine");
           action->setAction([this, i](){
               QProcess::startDetached("VBoxManage", {"startvm", uuids_[i]});
           });

           item->setActions({action});

           query->addMatch(item);
       }
   }
}
