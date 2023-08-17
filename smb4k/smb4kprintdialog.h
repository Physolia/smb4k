/*
 *  Print dialog
 *
 *  SPDX-FileCopyrightText: 2023 Alexander Reinholdt <alexander.reinholdt@kdemail.net>
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SMB4KPRINTDIALOG_H
#define SMB4KPRINTDIALOG_H

// application specific includes
#include "core/smb4kglobal.h"

// Qt includes
#include <QDialog>
#include <QPushButton>

class Q_DECL_EXPORT Smb4KPrintDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    Smb4KPrintDialog(QWidget *parent = nullptr);

    /**
     * Destructor
     */
    ~Smb4KPrintDialog();

    /**
     * Set the printer share to which should be printed
     *
     * @param printer       The printer share
     */
    void setPrinterShare(SharePtr printer);

protected Q_SLOTS:
    void slotCancelButtonClicked(bool checked);
    void slotPrintButtonClicked(bool checked);

private:
    QPushButton *m_cancelButton;
    QPushButton *m_printButton;
};

#endif
