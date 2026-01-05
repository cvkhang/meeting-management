#ifndef GROUPDETAILDIALOG_H
#define GROUPDETAILDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include "networkmanager.h"

class GroupDetailDialog : public QDialog {
    Q_OBJECT

public:
    explicit GroupDetailDialog(NetworkManager *netManager, int groupId, QWidget *parent = nullptr);

private:
    void loadGroupDetails();
    
    NetworkManager *m_networkManager;
    int m_groupId;
    
    QLabel *m_titleLabel;
    QTableWidget *m_membersTable;
    QPushButton *m_closeBtn;
};

#endif // GROUPDETAILDIALOG_H
