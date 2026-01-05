#include "groupdetaildialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

GroupDetailDialog::GroupDetailDialog(NetworkManager *netManager, int groupId, QWidget *parent)
    : QDialog(parent)
    , m_networkManager(netManager)
    , m_groupId(groupId)
{
    setWindowTitle("Chi tiết nhóm");
    setModal(true);
    setMinimumSize(500, 400);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    // Title
    m_titleLabel = new QLabel("Đang tải...");
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    layout->addWidget(m_titleLabel);
    
    // Members table
    m_membersTable = new QTableWidget(0, 2);
    m_membersTable->setHorizontalHeaderLabels({"Tên", "Vai trò"});
    m_membersTable->horizontalHeader()->setStretchLastSection(true);
    m_membersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_membersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_membersTable);
    
    // Close button
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    m_closeBtn = new QPushButton("Đóng");
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(m_closeBtn);
    layout->addLayout(btnLayout);
    
    // Load data
    loadGroupDetails();
}

void GroupDetailDialog::loadGroupDetails() {
    QString cmd = QString("VIEW_GROUP_DETAIL|token=%1;group_id=%2\r\n")
                      .arg(m_networkManager->getToken())
                      .arg(m_groupId);
    
    QString response = m_networkManager->sendRequest(cmd);
    int status = NetworkManager::getStatusCode(response);
    
    if (status == 200) {
        QString groupName = NetworkManager::getValue(response, "group_name");
        QString membersStr = NetworkManager::getValue(response, "members");
        
        m_titleLabel->setText(QString("Nhóm: %1").arg(groupName));
        
        // Parse members: format is "user_id,full_name,role#..."
        QStringList membersList = membersStr.split('#', Qt::SkipEmptyParts);
        
        m_membersTable->setRowCount(membersList.size());
        
        for (int i = 0; i < membersList.size(); ++i) {
            QStringList parts = membersList[i].split(',');
            if (parts.size() >= 3) {
                QString fullName = parts[1];
                int role = parts[2].toInt();
                QString roleStr = (role == 1) ? "Admin" : "Thành viên";
                
                m_membersTable->setItem(i, 0, new QTableWidgetItem(fullName));
                
                QTableWidgetItem *roleItem = new QTableWidgetItem(roleStr);
                if (role == 1) {
                    roleItem->setForeground(QBrush(QColor(255, 140, 0))); // Orange for admin
                }
                m_membersTable->setItem(i, 1, roleItem);
            }
        }
        
        m_membersTable->resizeColumnsToContents();
    } else {
        m_titleLabel->setText("Lỗi: Không thể tải thông tin nhóm");
        QMessageBox::warning(this, "Lỗi", "Không thể tải thông tin nhóm!");
    }
}
