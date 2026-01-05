#include "teacherwidget.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QDialog>

TeacherWidget::TeacherWidget(NetworkManager *nm, QWidget *parent)
    : QWidget(parent), m_networkManager(nm), m_currentMinuteId(0) {
    setupUi();
}

void TeacherWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Header
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("Menu Giảng Viên");
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #2196F3;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    
    QPushButton *logoutBtn = new QPushButton("Đăng xuất");
    logoutBtn->setStyleSheet("background-color: #f44336; color: white; padding: 8px 16px;");
    connect(logoutBtn, &QPushButton::clicked, this, &TeacherWidget::onLogout);
    headerLayout->addWidget(logoutBtn);
    
    mainLayout->addLayout(headerLayout);

    // Tab widget
    m_tabWidget = new QTabWidget();
    
    // === TAB 1: Slot Management ===
    QWidget *slotsTab = new QWidget();
    QVBoxLayout *slotsLayout = new QVBoxLayout(slotsTab);

    // My slots
    QGroupBox *mySlotsGroup = new QGroupBox("Các slot đã tạo");
    QVBoxLayout *msLayout = new QVBoxLayout(mySlotsGroup);
    
    QPushButton *refreshSlotsBtn = new QPushButton("Tải danh sách");
    connect(refreshSlotsBtn, &QPushButton::clicked, this, &TeacherWidget::onViewMySlots);
    msLayout->addWidget(refreshSlotsBtn);
    
    m_slotsTable = new QTableWidget();
    m_slotsTable->setColumnCount(8);
    m_slotsTable->setHorizontalHeaderLabels({"ID", "Ngày", "Bắt đầu", "Kết thúc", "Loại", "Max", "Trạng thái", "Actions"});
    m_slotsTable->horizontalHeader()->setStretchLastSection(true);
    m_slotsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    msLayout->addWidget(m_slotsTable);
    
    QHBoxLayout *deleteLayout = new QHBoxLayout();
    deleteLayout->addWidget(new QLabel("Slot ID:"));
    m_slotIdSpin = new QSpinBox();
    m_slotIdSpin->setRange(1, 9999);
    deleteLayout->addWidget(m_slotIdSpin);
    QPushButton *editSlotBtn = new QPushButton("Sửa slot");
    editSlotBtn->setStyleSheet("background-color: #FF9800; color: white;");
    connect(editSlotBtn, &QPushButton::clicked, this, &TeacherWidget::onEditSlot);
    deleteLayout->addWidget(editSlotBtn);
    
    QPushButton *deleteSlotBtn = new QPushButton("Xóa slot");
    deleteSlotBtn->setStyleSheet("background-color: #f44336; color: white;");
    connect(deleteSlotBtn, &QPushButton::clicked, this, &TeacherWidget::onDeleteSlot);
    deleteLayout->addWidget(deleteSlotBtn);
    deleteLayout->addStretch();
    msLayout->addLayout(deleteLayout);
    
    slotsLayout->addWidget(mySlotsGroup);

    // Create slot
    QGroupBox *createSlotGroup = new QGroupBox("Tạo slot mới");
    QVBoxLayout *csLayout = new QVBoxLayout(createSlotGroup);
    
    QHBoxLayout *row1 = new QHBoxLayout();
    row1->addWidget(new QLabel("Ngày:"));
    m_dateEdit = new QDateEdit(QDate::currentDate());
    m_dateEdit->setCalendarPopup(true);
    row1->addWidget(m_dateEdit);
    row1->addWidget(new QLabel("Bắt đầu:"));
    m_startTimeEdit = new QTimeEdit(QTime(9, 0));
    row1->addWidget(m_startTimeEdit);
    row1->addWidget(new QLabel("Kết thúc:"));
    m_endTimeEdit = new QTimeEdit(QTime(10, 0));
    row1->addWidget(m_endTimeEdit);
    csLayout->addLayout(row1);
    
    QHBoxLayout *row2 = new QHBoxLayout();
    row2->addWidget(new QLabel("Loại:"));
    m_slotTypeCombo = new QComboBox();
    m_slotTypeCombo->addItem("Cá nhân", "INDIVIDUAL");
    m_slotTypeCombo->addItem("Nhóm", "GROUP");
    m_slotTypeCombo->addItem("Cả hai", "BOTH");
    row2->addWidget(m_slotTypeCombo);
    row2->addWidget(new QLabel("Số người tối đa:"));
    m_maxGroupSpin = new QSpinBox();
    m_maxGroupSpin->setRange(1, 50);
    m_maxGroupSpin->setValue(5);
    row2->addWidget(m_maxGroupSpin);
    QPushButton *createSlotBtn = new QPushButton("Tạo slot");
    createSlotBtn->setStyleSheet("background-color: #4CAF50; color: white;");
    connect(createSlotBtn, &QPushButton::clicked, this, &TeacherWidget::onCreateSlot);
    row2->addWidget(createSlotBtn);
    csLayout->addLayout(row2);
    
    slotsLayout->addWidget(createSlotGroup);
    m_tabWidget->addTab(slotsTab, "Quản lý Slot");

    // === TAB 2: Meetings ===
    QWidget *meetingsTab = new QWidget();
    QVBoxLayout *meetingsLayout = new QVBoxLayout(meetingsTab);

    QPushButton *refreshMeetingsBtn = new QPushButton("Tải lịch hẹn");
    connect(refreshMeetingsBtn, &QPushButton::clicked, this, &TeacherWidget::onViewMeetings);
    meetingsLayout->addWidget(refreshMeetingsBtn);
    
    m_meetingsTable = new QTableWidget();
    m_meetingsTable->setColumnCount(7);
    m_meetingsTable->setHorizontalHeaderLabels({"ID", "Ngày", "Bắt đầu", "Kết thúc", "Loại", "Trạng thái", "Action"});
    m_meetingsTable->horizontalHeader()->setStretchLastSection(true);
    m_meetingsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    meetingsLayout->addWidget(m_meetingsTable);
    
    QGroupBox *detailGroup = new QGroupBox("Chi tiết & Biên bản");
    QVBoxLayout *dgLayout = new QVBoxLayout(detailGroup);
    
    QHBoxLayout *meetingActions = new QHBoxLayout();
    meetingActions->addWidget(new QLabel("Meeting ID:"));
    m_meetingIdSpin = new QSpinBox();
    m_meetingIdSpin->setRange(1, 9999);
    meetingActions->addWidget(m_meetingIdSpin);
    QPushButton *viewDetailBtn = new QPushButton("Xem chi tiết");
    connect(viewDetailBtn, &QPushButton::clicked, this, &TeacherWidget::onViewMeetingDetail);
    meetingActions->addWidget(viewDetailBtn);
    meetingActions->addStretch();
    dgLayout->addLayout(meetingActions);
    
    dgLayout->addWidget(new QLabel("Nội dung biên bản:"));
    m_minutesEdit = new QTextEdit();
    m_minutesEdit->setMaximumHeight(100);
    dgLayout->addWidget(m_minutesEdit);
    
    QHBoxLayout *minutesButtons = new QHBoxLayout();
    
    QPushButton *viewMinutesBtn = new QPushButton("Xem biên bản");
    viewMinutesBtn->setStyleSheet("background-color: #2196F3; color: white;");
    connect(viewMinutesBtn, &QPushButton::clicked, this, &TeacherWidget::onViewMinutes);
    minutesButtons->addWidget(viewMinutesBtn);
    
    QPushButton *writeMinutesBtn = new QPushButton("Lưu biên bản");
    writeMinutesBtn->setStyleSheet("background-color: #4CAF50; color: white;");
    connect(writeMinutesBtn, &QPushButton::clicked, this, &TeacherWidget::onWriteMinutes);
    minutesButtons->addWidget(writeMinutesBtn);
    
    QPushButton *updateMinutesBtn = new QPushButton("Cập nhật biên bản");
    updateMinutesBtn->setStyleSheet("background-color: #FF9800; color: white;");
    connect(updateMinutesBtn, &QPushButton::clicked, this, &TeacherWidget::onUpdateMinutes);
    minutesButtons->addWidget(updateMinutesBtn);
    
    dgLayout->addLayout(minutesButtons);
    
    meetingsLayout->addWidget(detailGroup);
    
    // === History Section (moved into Meetings tab) ===
    QGroupBox *historyGroup = new QGroupBox("Lịch sử cuộc họp");
    QVBoxLayout *historyLayout = new QVBoxLayout(historyGroup);
    
    QPushButton *refreshHistoryBtn = new QPushButton("Tải lịch sử");
    connect(refreshHistoryBtn, &QPushButton::clicked, this, &TeacherWidget::onViewHistory);
    historyLayout->addWidget(refreshHistoryBtn);
    
    m_historyTable = new QTableWidget();
    m_historyTable->setColumnCount(7);
    m_historyTable->setHorizontalHeaderLabels({"ID", "Ngày", "Thời gian", "Đối tượng", "Loại", "Biên bản", "Action"});
    m_historyTable->horizontalHeader()->setStretchLastSection(true);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    historyLayout->addWidget(m_historyTable);
    
    meetingsLayout->addWidget(historyGroup);
    m_tabWidget->addTab(meetingsTab, "Lịch hẹn");

    mainLayout->addWidget(m_tabWidget);
}

void TeacherWidget::refresh() {
    onViewMySlots();
    onViewMeetings();
    onViewHistory();
}

void TeacherWidget::clearTable(QTableWidget *table) {
    table->setRowCount(0);
}

void TeacherWidget::onViewMySlots() {
    QString cmd = QString("VIEW_SLOTS|teacher_id=%1\r\n").arg(m_networkManager->getUserId());
    QString response = m_networkManager->sendRequest(cmd);
    
    clearTable(m_slotsTable);
    QString slotsData = NetworkManager::getValue(response, "slots");
    
    if (!slotsData.isEmpty()) {
        QStringList list = slotsData.split('#');
        for (const QString &s : list) {
            QStringList parts = s.split(',');
            // Format: slot_id,date,start_time,end_time,slot_type,max_group_size,status
            if (parts.size() >= 7) {
                int row = m_slotsTable->rowCount();
                m_slotsTable->insertRow(row);
                
                int slotId = parts[0].toInt();
                QString status = parts[6];
                
                m_slotsTable->setItem(row, 0, new QTableWidgetItem(parts[0]));  // ID
                m_slotsTable->setItem(row, 1, new QTableWidgetItem(parts[1]));  // Date
                m_slotsTable->setItem(row, 2, new QTableWidgetItem(parts[2]));  // Start
                m_slotsTable->setItem(row, 3, new QTableWidgetItem(parts[3]));  // End
                m_slotsTable->setItem(row, 4, new QTableWidgetItem(parts[4]));  // Type
                m_slotsTable->setItem(row, 5, new QTableWidgetItem(parts[5]));  // Max
                
                // Status with color coding
                QTableWidgetItem *statusItem = new QTableWidgetItem(status == "AVAILABLE" ? "Còn trống" : "Đã đặt");
                if (status == "AVAILABLE") {
                    statusItem->setForeground(QColor("#4CAF50")); // Green
                } else {
                    statusItem->setForeground(QColor("#f44336")); // Red
                }
                m_slotsTable->setItem(row, 6, statusItem);
                
                // Action buttons
                QWidget *actionsWidget = new QWidget();
                QHBoxLayout *layout = new QHBoxLayout(actionsWidget);
                layout->setContentsMargins(2, 2, 2, 2);
                layout->setSpacing(4);
                
                if (status == "AVAILABLE") {
                    // Only allow edit/delete for available slots
                    QPushButton *editBtn = new QPushButton("Sửa");
                    editBtn->setStyleSheet("background-color: #FF9800; color: white; padding: 4px 8px;");
                    connect(editBtn, &QPushButton::clicked, [this, slotId]() {
                        m_slotIdSpin->setValue(slotId);
                        onEditSlot();
                    });
                    
                    QPushButton *deleteBtn = new QPushButton("Xóa");
                    deleteBtn->setStyleSheet("background-color: #f44336; color: white; padding: 4px 8px;");
                    connect(deleteBtn, &QPushButton::clicked, [this, slotId]() {
                        m_slotIdSpin->setValue(slotId);
                        onDeleteSlot();
                    });
                    
                    layout->addWidget(editBtn);
                    layout->addWidget(deleteBtn);
                } else {
                    // Booked slots cannot be edited/deleted
                    QLabel *label = new QLabel("Không thể sửa");
                    label->setStyleSheet("color: #999; font-style: italic;");
                    layout->addWidget(label);
                }
                
                m_slotsTable->setCellWidget(row, 7, actionsWidget);
            }
        }
    }
}

void TeacherWidget::onCreateSlot() {
    QString date = m_dateEdit->date().toString("yyyy-MM-dd");
    QString startTime = m_startTimeEdit->time().toString("HH:mm");
    QString endTime = m_endTimeEdit->time().toString("HH:mm");
    QString slotType = m_slotTypeCombo->currentData().toString();
    int maxGroup = m_maxGroupSpin->value();
    
    QString cmd = QString("DECLARE_SLOT|token=%1;date=%2;start_time=%3;end_time=%4;slot_type=%5;max_group_size=%6\r\n")
                      .arg(m_networkManager->getToken(), date, startTime, endTime, slotType)
                      .arg(maxGroup);
    QString response = m_networkManager->sendRequest(cmd);
    
    int status = NetworkManager::getStatusCode(response);
    if (status == 201) {
        QMessageBox::information(this, "Thành công", "Tạo slot thành công!");
        onViewMySlots();
    } else {
        QMessageBox::warning(this, "Lỗi", "Tạo slot thất bại!");
    }
}

void TeacherWidget::onEditSlot() {
    int slotId = m_slotIdSpin->value();
    
    // Create dialog for editing
    QDialog dialog(this);
    dialog.setWindowTitle("Sửa Slot");
    dialog.setMinimumWidth(400);
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    
    QHBoxLayout *row1 = new QHBoxLayout();
    row1->addWidget(new QLabel("Ngày:"));
    QDateEdit *dateEdit = new QDateEdit(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    row1->addWidget(dateEdit);
    layout->addLayout(row1);
    
    QHBoxLayout *row2 = new QHBoxLayout();
    row2->addWidget(new QLabel("Bắt đầu:"));
    QTimeEdit *startEdit = new QTimeEdit(QTime(9, 0));
    row2->addWidget(startEdit);
    row2->addWidget(new QLabel("Kết thúc:"));
    QTimeEdit *endEdit = new QTimeEdit(QTime(10, 0));
    row2->addWidget(endEdit);
    layout->addLayout(row2);
    
    QHBoxLayout *row3 = new QHBoxLayout();
    row3->addWidget(new QLabel("Loại:"));
    QComboBox *typeCombo = new QComboBox();
    typeCombo->addItem("Cá nhân", "INDIVIDUAL");
    typeCombo->addItem("Nhóm", "GROUP");
    typeCombo->addItem("Cả hai", "BOTH");
    row3->addWidget(typeCombo);
    row3->addWidget(new QLabel("Số người tối đa:"));
    QSpinBox *maxSpin = new QSpinBox();
    maxSpin->setRange(1, 50);
    maxSpin->setValue(5);
    row3->addWidget(maxSpin);
    layout->addLayout(row3);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *saveBtn = new QPushButton("Lưu");
    saveBtn->setStyleSheet("background-color: #4CAF50; color: white;");
    QPushButton *cancelBtn = new QPushButton("Hủy");
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);
    
    connect(saveBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    if (dialog.exec() == QDialog::Accepted) {
        QString cmd = QString("EDIT_SLOT|token=%1;slot_id=%2;action=UPDATE;date=%3;start_time=%4;end_time=%5;slot_type=%6;max_group_size=%7\r\n")
                          .arg(m_networkManager->getToken())
                          .arg(slotId)
                          .arg(dateEdit->date().toString("yyyy-MM-dd"))
                          .arg(startEdit->time().toString("HH:mm"))
                          .arg(endEdit->time().toString("HH:mm"))
                          .arg(typeCombo->currentData().toString())
                          .arg(maxSpin->value());
        QString response = m_networkManager->sendRequest(cmd);
        
        int status = NetworkManager::getStatusCode(response);
        if (status == 200) {
            QMessageBox::information(this, "Thành công", "Cập nhật slot thành công!");
            onViewMySlots();
        } else {
            QMessageBox::warning(this, "Lỗi", "Cập nhật slot thất bại!");
        }
    }
}

void TeacherWidget::onDeleteSlot() {
    int slotId = m_slotIdSpin->value();
    
    QString cmd = QString("EDIT_SLOT|token=%1;slot_id=%2;action=DELETE\r\n")
                      .arg(m_networkManager->getToken())
                      .arg(slotId);
    QString response = m_networkManager->sendRequest(cmd);
    
    int status = NetworkManager::getStatusCode(response);
    if (status == 200) {
        QMessageBox::information(this, "Thành công", "Xóa slot thành công!");
        onViewMySlots();
    } else {
        QMessageBox::warning(this, "Lỗi", "Xóa slot thất bại!");
    }
}

void TeacherWidget::onViewMeetings() {
    QString cmd = QString("VIEW_MEETINGS|token=%1\r\n").arg(m_networkManager->getToken());
    QString response = m_networkManager->sendRequest(cmd);
    
    clearTable(m_meetingsTable);
    QString meetings = NetworkManager::getValue(response, "meetings");
    
    if (!meetings.isEmpty()) {
        QStringList list = meetings.split('#');
        for (const QString &m : list) {
            QStringList parts = m.split(',');
            if (parts.size() >= 8) {
                int row = m_meetingsTable->rowCount();
                m_meetingsTable->insertRow(row);
                m_meetingsTable->setItem(row, 0, new QTableWidgetItem(parts[0]));
                m_meetingsTable->setItem(row, 1, new QTableWidgetItem(parts[1]));
                m_meetingsTable->setItem(row, 2, new QTableWidgetItem(parts[2]));
                m_meetingsTable->setItem(row, 3, new QTableWidgetItem(parts[3]));
                m_meetingsTable->setItem(row, 4, new QTableWidgetItem(parts[6]));
                m_meetingsTable->setItem(row, 5, new QTableWidgetItem(parts[7]));
                
                // Add complete button for BOOKED meetings
                if (parts[7] == "BOOKED") {
                    QPushButton *completeBtn = new QPushButton("Hoàn thành");
                    completeBtn->setStyleSheet("background-color: #4CAF50; color: white;");
                    int meetingId = parts[0].toInt();
                    connect(completeBtn, &QPushButton::clicked, [this, meetingId]() {
                        m_meetingIdSpin->setValue(meetingId);
                        onCompleteMeeting();
                    });
                    m_meetingsTable->setCellWidget(row, 6, completeBtn);
                }
            }
        }
    }
}

void TeacherWidget::onViewMeetingDetail() {
    int meetingId = m_meetingIdSpin->value();
    
    QString cmd = QString("VIEW_MEETING_DETAIL|token=%1;meeting_id=%2\r\n")
                      .arg(m_networkManager->getToken())
                      .arg(meetingId);
    QString response = m_networkManager->sendRequest(cmd);
    
    int status = NetworkManager::getStatusCode(response);
    if (status == 200) {
        QString date = NetworkManager::getValue(response, "date");
        QString st = NetworkManager::getValue(response, "start_time");
        QString et = NetworkManager::getValue(response, "end_time");
        QString type = NetworkManager::getValue(response, "meeting_type");
        QString mstatus = NetworkManager::getValue(response, "status");
        QString hasMin = NetworkManager::getValue(response, "has_minutes");
        
        QString info = QString("Ngày: %1\nThời gian: %2 - %3\nLoại: %4\nTrạng thái: %5\nBiên bản: %6")
                           .arg(date, st, et, type, mstatus, hasMin == "1" ? "Đã có" : "Chưa có");
        QMessageBox::information(this, "Chi tiết cuộc họp", info);
    } else {
        QMessageBox::warning(this, "Lỗi", "Không tìm thấy cuộc họp!");
    }
}

void TeacherWidget::onWriteMinutes() {
    int meetingId = m_meetingIdSpin->value();
    QString content = m_minutesEdit->toPlainText().trimmed();
    
    if (content.isEmpty()) {
        QMessageBox::warning(this, "Lỗi", "Vui lòng nhập nội dung biên bản!");
        return;
    }
    
    QString cmd = QString("SAVE_MINUTES|token=%1;meeting_id=%2;content=%3\r\n")
                      .arg(m_networkManager->getToken())
                      .arg(meetingId)
                      .arg(content);
    QString response = m_networkManager->sendRequest(cmd);
    
    int status = NetworkManager::getStatusCode(response);
    if (status == 201 || status == 200) {
        QMessageBox::information(this, "Thành công", "Lưu biên bản thành công!");
        m_minutesEdit->clear();
    } else {
        QMessageBox::warning(this, "Lỗi", "Lưu biên bản thất bại!");
    }
}

void TeacherWidget::onViewMinutes() {
    int meetingId = m_meetingIdSpin->value();
    
    QString cmd = QString("VIEW_MINUTES|token=%1;meeting_id=%2\r\n")
                      .arg(m_networkManager->getToken())
                      .arg(meetingId);
    QString response = m_networkManager->sendRequest(cmd);
    
    int status = NetworkManager::getStatusCode(response);
    if (status == 200) {
        QString minuteId = NetworkManager::getValue(response, "minute_id");
        
        if (minuteId == "0") {
            m_currentMinuteId = 0;
            QMessageBox::information(this, "Thông báo", "Cuộc họp này chưa có biên bản.");
            m_minutesEdit->clear();
        } else {
            m_currentMinuteId = minuteId.toInt();
            QString content = NetworkManager::getValue(response, "content");
            QString createdAt = NetworkManager::getValue(response, "created_at");
            QString updatedAt = NetworkManager::getValue(response, "updated_at");
            
            // Load content into text edit
            m_minutesEdit->setPlainText(content);
            
            // Show info dialog with proper NULL handling
            QString createdAtDisplay = createdAt.isEmpty() ? "N/A" : createdAt;
            QString updatedAtDisplay = updatedAt.isEmpty() || updatedAt == createdAt ? "Chưa cập nhật" : updatedAt;
            QString info = QString("Biên bản #%1\nTạo lúc: %2\nCập nhật: %3")
                               .arg(minuteId)
                               .arg(createdAtDisplay)
                               .arg(updatedAtDisplay);
            QMessageBox::information(this, "Thông tin biên bản", info);
        }
    } else if (status == 404) {
        QMessageBox::warning(this, "Lỗi", "Không tìm thấy cuộc họp!");
    } else {
        QMessageBox::warning(this, "Lỗi", "Không thể xem biên bản!");
    }
}

void TeacherWidget::onUpdateMinutes() {
    if (m_currentMinuteId == 0) {
        QMessageBox::warning(this, "Lỗi", "Vui lòng xem biên bản trước khi cập nhật!");
        return;
    }
    
    QString content = m_minutesEdit->toPlainText().trimmed();
    
    if (content.isEmpty()) {
        QMessageBox::warning(this, "Lỗi", "Vui lòng nhập nội dung biên bản!");
        return;
    }
    
    QString cmd = QString("UPDATE_MINUTES|token=%1;minute_id=%2;content=%3\r\n")
                      .arg(m_networkManager->getToken())
                      .arg(m_currentMinuteId)
                      .arg(content);
    QString response = m_networkManager->sendRequest(cmd);
    
    int status = NetworkManager::getStatusCode(response);
    if (status == 200) {
        QMessageBox::information(this, "Thành công", "Cập nhật biên bản thành công!");
        onViewMinutes(); // Refresh to show updated timestamp
    } else {
        QMessageBox::warning(this, "Lỗi", "Cập nhật biên bản thất bại!");
    }
}

void TeacherWidget::onCompleteMeeting() {
    int meetingId = m_meetingIdSpin->value();
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Xác nhận", 
        QString("Đánh dấu cuộc họp #%1 là đã hoàn thành?").arg(meetingId),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QString cmd = QString("COMPLETE_MEETING|token=%1;meeting_id=%2\r\n")
                          .arg(m_networkManager->getToken())
                          .arg(meetingId);
        QString response = m_networkManager->sendRequest(cmd);
        
        int status = NetworkManager::getStatusCode(response);
        if (status == 200) {
            QMessageBox::information(this, "Thành công", "Đã hoàn thành cuộc họp!");
            onViewMeetings(); // Refresh
        } else if (status == 403) {
            QMessageBox::warning(this, "Lỗi", "Bạn không phải giảng viên của cuộc họp này!");
        } else if (status == 400) {
            QMessageBox::warning(this, "Lỗi", "Chỉ có thể hoàn thành meeting đang BOOKED!");
        } else {
            QMessageBox::warning(this, "Lỗi", "Không thể hoàn thành cuộc họp!");
        }
    }
}

void TeacherWidget::onViewHistory() {
    QString cmd = QString("VIEW_MEETING_HISTORY|token=%1\r\n").arg(m_networkManager->getToken());
    QString response = m_networkManager->sendRequest(cmd);
    
    clearTable(m_historyTable);
    int status = NetworkManager::getStatusCode(response);
    
    if (status == 200) {
        QString history = NetworkManager::getValue(response, "history");
        
        if (!history.isEmpty()) {
            QStringList list = history.split('#');
            for (const QString &h : list) {
                QStringList parts = h.split(',');
                // Format: meeting_id,date,start_time,end_time,partner_name,type,has_minutes
                if (parts.size() >= 7) {
                    int row = m_historyTable->rowCount();
                    m_historyTable->insertRow(row);
                    
                    m_historyTable->setItem(row, 0, new QTableWidgetItem(parts[0]));  // ID
                    m_historyTable->setItem(row, 1, new QTableWidgetItem(parts[1]));  // Date
                    m_historyTable->setItem(row, 2, new QTableWidgetItem(parts[2] + " - " + parts[3]));  // Time
                    m_historyTable->setItem(row, 3, new QTableWidgetItem(parts[4]));  // Partner name
                    m_historyTable->setItem(row, 4, new QTableWidgetItem(parts[5]));  // Type
                    
                    // Minutes indicator
                    QString minutesStatus = parts[6] == "1" ? "Đã có" : "Chưa có";
                    m_historyTable->setItem(row, 5, new QTableWidgetItem(minutesStatus));
                    
                    // Add view button if has minutes
                    if (parts[6] == "1") {
                        QPushButton *viewBtn = new QPushButton("Xem");
                        viewBtn->setStyleSheet("background-color: #2196F3; color: white; padding: 4px 8px;");
                        int meetingId = parts[0].toInt();
                        connect(viewBtn, &QPushButton::clicked, [this, meetingId]() {
                            m_meetingIdSpin->setValue(meetingId);
                            m_tabWidget->setCurrentIndex(1);  // Switch to Meetings tab
                            onViewMinutes();
                        });
                        m_historyTable->setCellWidget(row, 6, viewBtn);
                    }
                }
            }
        }
    }
}

void TeacherWidget::onLogout() {
    emit logoutRequested();
}
