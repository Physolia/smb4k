/*
    This is the shares view of Smb4K.
    -------------------
    begin                : Mo Dez 4 2006
    SPDX-FileCopyrightText: 2006-2021 Alexander Reinholdt
    email                : alexander.reinholdt@kdemail.net
*/

/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
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

#ifndef SMB4KSHARESVIEW_H
#define SMB4KSHARESVIEW_H

// Qt includes
#include <QListWidget>
#include <QMimeData>
#include <QTimer>

// forward declarations
class Smb4KSharesViewItem;
class Smb4KToolTip;

/**
 * This widget class provides the shares view of Smb4K.
 *
 * @author Alexander Reinholdt <alexander.reinholdt@kdemail.net>
 */

class Smb4KSharesView : public QListWidget
{
    Q_OBJECT

public:
    /**
     * The constructor.
     *
     * @param parent        The parent widget
     */
    explicit Smb4KSharesView(QWidget *parent = 0);

    /**
     * The destructor.
     */
    ~Smb4KSharesView();

    /**
     * Set the view mode.
     *
     * @param mode          The view mode
     * @param iconSize      The size of the icons
     */
    void setViewMode(ViewMode mode, int iconSize);

    /**
     * The tooltip
     */
    Smb4KToolTip *toolTip();

signals:
    /**
     * This signal is emitted when something has been dropped onto
     * @p item and the drop event was accepted.
     *
     * @param item          The item on which something has been dropped.
     *
     * @param e             The drop event.
     */
    void acceptedDropEvent(Smb4KSharesViewItem *item, QDropEvent *e);

protected:
    /**
     * Reimplemented from QListWidget.
     */
    bool event(QEvent *e) override;

    /**
     * Reimplemented from QAbstractItemView. This function handles
     * mouse press events.
     *
     * @param e             The mouse event object
     */
    void mousePressEvent(QMouseEvent *e) override;

    /**
     * Reimplemented from QAbstractItemView. This function handles
     * mouse move events.
     *
     * @param e             The mouse event object
     */
    void mouseMoveEvent(QMouseEvent *e) override;

    /**
     * Reimplemented to allow dragging and dropping.
     *
     * @param e             The drag event
     */
    virtual void dragEnterEvent(QDragEnterEvent *e) override;

    /**
     * Reimplemented to allow dragging and dropping.
     *
     * @param e             The drag move event
     */
    void dragMoveEvent(QDragMoveEvent *e) override;

    /**
     * Reimplemented to allow dragging and dropping.
     *
     * @param e             The drop event
     */
    void dropEvent(QDropEvent *e) override;

    /**
     * Reimplemented to allow only the copy drop action.
     */
    Qt::DropActions supportedDropActions() const override;

    /**
     * Reimplemented to allow dragging.
     */
    QMimeData *mimeData(const QList<QListWidgetItem *> list) const override;

    /**
     * Reimplemented to allow dragging.
     */
    void startDrag(Qt::DropActions supported) override;

private:
    /**
     * The tool top widget
     */
    Smb4KToolTip *m_toolTip;
};

#endif
