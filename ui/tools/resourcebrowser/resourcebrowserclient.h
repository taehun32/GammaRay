/*
 * <file>
 *
 * This file is part of GammaRay, the Qt application inspection and
 * manipulation tool.
 *
 * Copyright (C) 2013-2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
 * Author: Milian Wolff <milian.wolff@kdab.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef RESOURCEBROWSERCLIENT_H
#define RESOURCEBROWSERCLIENT_H

#include <common/resourcebrowserinterface.h>

namespace GammaRay {

class ResourceBrowserClient : public ResourceBrowserInterface
{
  Q_OBJECT
  Q_INTERFACES(GammaRay::ResourceBrowserInterface)
  public:
    explicit ResourceBrowserClient(QObject *parent);
    virtual ~ResourceBrowserClient();
};

}

#endif // RESOURCEBROWSERCLIENT_H
