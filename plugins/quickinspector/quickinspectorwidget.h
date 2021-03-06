/*
  quickinspectorwidget.h

  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Volker Krause <volker.krause@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GAMMARAY_QUICKINSPECTORWIDGET_H
#define GAMMARAY_QUICKINSPECTORWIDGET_H

#include <ui/tooluifactory.h>

#include <QWidget>

class QLabel;
class QImage;
class QItemSelection;

namespace GammaRay {

class QuickInspectorInterface;

namespace Ui {
  class QuickInspectorWidget;
}

class QuickInspectorWidget : public QWidget
{
  Q_OBJECT
  public:
    explicit QuickInspectorWidget(QWidget *parent = 0);
    ~QuickInspectorWidget();

  private slots:
    void sceneRendered(const QImage &img);
    void itemSelectionChanged(const QItemSelection &selection);

  private:
    QScopedPointer<Ui::QuickInspectorWidget> ui;
    QuickInspectorInterface* m_interface;
    QLabel *m_sceneImage;
};

class QuickInspectorUiFactory : public QObject, public StandardToolUiFactory<QuickInspectorWidget>
{
  Q_OBJECT
  Q_INTERFACES(GammaRay::ToolUiFactory)
  Q_PLUGIN_METADATA(IID "com.kdab.gammaray.QuickInspectorUi")
};

}

#endif
