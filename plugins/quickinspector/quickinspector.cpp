/*
  qmlsupport.cpp

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

#include "quickinspector.h"
#include "quickitemmodel.h"

#include <common/objectbroker.h>

#include <core/metaobject.h>
#include <core/metaobjectrepository.h>
#include <core/objecttypefilterproxymodel.h>
#include <core/probeinterface.h>
#include <core/propertycontroller.h>
#include <core/remote/server.h>
#include <core/singlecolumnobjectproxymodel.h>

#include <QQuickItem>
#include <QQuickView>

#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlError>

#include <QDebug>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QPainter>

Q_DECLARE_METATYPE(QQmlError)

using namespace GammaRay;

QuickInspector::QuickInspector(ProbeInterface* probe, QObject* parent) :
  QuickInspectorInterface(parent),
  m_probe(probe),
  m_itemModel(new QuickItemModel(this)),
  m_propertyController(new PropertyController("com.kdab.GammaRay.QuickItem", this)),
  m_clientConnected(false)
{
  Server::instance()->registerMonitorNotifier(Endpoint::instance()->objectAddress(objectName()), this, "clientConnectedChanged");

  registerMetaTypes();
  probe->installGlobalEventFilter(this);

  QAbstractProxyModel* windowModel = new ObjectTypeFilterProxyModel<QQuickWindow>(this);
  windowModel->setSourceModel(probe->objectListModel());
  QAbstractProxyModel* proxy = new SingleColumnObjectProxyModel(this);
  proxy->setSourceModel(windowModel);
  m_windowModel = proxy;
  probe->registerModel("com.kdab.GammaRay.QuickWindowModel", m_windowModel);
  probe->registerModel("com.kdab.GammaRay.QuickItemModel", m_itemModel);

  connect(probe->probe(), SIGNAL(objectCreated(QObject*)), m_itemModel, SLOT(objectAdded(QObject*)));
  connect(probe->probe(), SIGNAL(objectDestroyed(QObject*)), m_itemModel, SLOT(objectRemoved(QObject*)));

  m_itemSelectionModel = ObjectBroker::selectionModel(m_itemModel);
  connect(m_itemSelectionModel, &QItemSelectionModel::selectionChanged, this, &QuickInspector::itemSelectionChanged);
}

QuickInspector::~QuickInspector()
{
}

void QuickInspector::selectWindow(int index)
{
  const QModelIndex mi = m_windowModel->index(index, 0);
  QQuickWindow* window = mi.data(ObjectModel::ObjectRole).value<QQuickWindow*>();
  selectWindow(window);
}

void QuickInspector::selectWindow(QQuickWindow* window)
{
  if (m_window) {
    disconnect(m_window, 0, this, 0);
  }

  m_window = window;
  m_itemModel->setWindow(window);

  if (m_window) {
    connect(window, &QQuickWindow::frameSwapped, this, &QuickInspector::frameSwapped);
  }

  renderScene();
}

void QuickInspector::selectItem(QQuickItem* item)
{
  const QAbstractItemModel *model = m_itemModel;
  const QModelIndexList indexList = model->match(model->index(0, 0), ObjectModel::ObjectRole,
    QVariant::fromValue<QQuickItem*>(item), 1, Qt::MatchExactly | Qt::MatchRecursive);
  if (indexList.isEmpty())
    return;

  const QModelIndex index = indexList.first();
  m_itemSelectionModel->select( index, QItemSelectionModel::Select | QItemSelectionModel::Clear |
    QItemSelectionModel::Rows | QItemSelectionModel::Current);
}

void QuickInspector::renderScene()
{
  if (!m_clientConnected || !m_window)
    return;

  QImage img = m_window->grabWindow();
  if (m_currentItem) {
    QPainter p(&img);

    // bounding box
    const QRectF itemRect(0, 0, m_currentItem->width(), m_currentItem->height());
    p.setPen(Qt::blue);
    p.drawRect(m_currentItem->mapRectToScene(itemRect));

    // children rect
    p.setPen(Qt::cyan);
    p.drawRect(m_currentItem->mapRectToScene(m_currentItem->childrenRect()));

    // transform origin
    p.setPen(Qt::red);
    const QPointF tfo = m_currentItem->property("transformOriginPoint").toPointF();
    p.drawEllipse(m_currentItem->mapToScene(tfo), 2, 2);
  }

  emit sceneRendered(img);
}

void QuickInspector::frameSwapped()
{
  // ### just for testing, we need to rate-limit that and only update if the client actually wants that
  renderScene();
}

void QuickInspector::itemSelectionChanged(const QItemSelection& selection)
{
  if (selection.isEmpty())
    return;
  const QModelIndex index = selection.first().topLeft();
  m_currentItem = index.data(ObjectModel::ObjectRole).value<QQuickItem*>();
  m_propertyController->setObject(m_currentItem);
  renderScene();
}

void QuickInspector::clientConnectedChanged(bool connected)
{
  m_clientConnected = connected;
  renderScene();
}

QQuickItem* QuickInspector::recursiveChiltAt(QQuickItem* parent, const QPointF& pos) const
{
  Q_ASSERT(parent);
  QQuickItem *child = parent->childAt(pos.x(), pos.y());
  if (child)
    return recursiveChiltAt(child, parent->mapToItem(child, pos));
  return parent;
}

bool QuickInspector::eventFilter(QObject *receiver, QEvent *event)
{
  if (event->type() == QEvent::MouseButtonRelease) {
    QMouseEvent *mouseEv = static_cast<QMouseEvent*>(event);
    if (mouseEv->button() == Qt::LeftButton &&
        mouseEv->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
      QQuickWindow *window = qobject_cast<QQuickWindow*>(receiver);
      if (window && window->contentItem()) {
        QQuickItem *item = recursiveChiltAt(window->contentItem(), mouseEv->pos());
        m_probe->selectObject(item);
        selectItem(item);
      }
    }
  }

  return QObject::eventFilter(receiver, event);
}

void QuickInspector::registerMetaTypes()
{
  MetaObject *mo = 0;
  MO_ADD_METAOBJECT1(QQuickWindow, QWindow);
  MO_ADD_PROPERTY   (QQuickWindow, bool, clearBeforeRendering, setClearBeforeRendering);
  MO_ADD_PROPERTY   (QQuickWindow, bool, isPersistentOpenGLContext, setPersistentOpenGLContext);
  MO_ADD_PROPERTY   (QQuickWindow, bool, isPersistentSceneGraph, setPersistentSceneGraph);
  MO_ADD_PROPERTY_RO(QQuickWindow, QQuickItem*, mouseGrabberItem);
  MO_ADD_PROPERTY_RO(QQuickWindow, QOpenGLContext*, openglContext);
  MO_ADD_PROPERTY_RO(QQuickWindow, uint, renderTargetId);

  MO_ADD_METAOBJECT1(QQuickView, QQuickWindow);
  MO_ADD_PROPERTY_RO(QQuickView, QQmlEngine*, engine);
  MO_ADD_PROPERTY_RO(QQuickView, QList<QQmlError>, errors);
  MO_ADD_PROPERTY_RO(QQuickView, QSize, initialSize);
  MO_ADD_PROPERTY_RO(QQuickView, QQmlContext*, rootContext);
  MO_ADD_PROPERTY_RO(QQuickView, QQuickItem*, rootObject);
}

QString QuickInspectorFactory::name() const
{
  return tr("Quick Scenes");
}
