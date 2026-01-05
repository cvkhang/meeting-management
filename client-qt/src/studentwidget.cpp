#include "studentwidget.h"
#include <QMessageBox>
#include <algorithm>
#include <QInputDialog>
#include <QHeaderView>

StudentWidget::StudentWidget(NetworkManager *networkManager, QWidget *parent)
    : QWidget(parent)
    , m_networkManager(networkManager)
{
    setupUi();
}

void StudentWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Header
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("Menu Sinh Viên");
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #4CAF50;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    
    QPushButton *logoutBtn = new QPushButton("Đăng xuất");
    logoutBtn->setStyleSheet("background-color: #f44336; color: white; padding: 8px 16px;");
    connect(logoutBtn, &QPushButton::clicked, this, &StudentWidget::onLogout);
    headerLayout->addWidget(logoutBtn);
    
    mainLayout->addLayout(headerLayout);

    // Tab widget
    m_tabWidget = new QTabWidget();
    
    // === TAB 1: Teachers & Slots ===
    QWidget *teachersTab = new QWidget();
    QVBoxLayout *teachersLayout = new QVBoxLayout(teachersTab);

    // Teachers list
    QGroupBox *teachersGroup = new QGroupBox("Danh sách Giảng viên");
    QVBoxLayout *tgLayout = new QVBoxLayout(teachersGroup);
    
    QPushButton *refreshTeachersBtn = new QPushButton("Tải danh sách");
    connect(refreshTeachersBtn, &QPushButton::clicked, this, &StudentWidget::onViewTeachers);
    tgLayout->addWidget(refreshTeachersBtn);
    
    m_teachersTable = new QTableWidget();
    m_teachersTable->setColumnCount(3);
    m_teachersTable->setHorizontalHeaderLabels({"ID", "Họ tên", "Slot trống"});
    m_teachersTable->horizontalHeader()->setStretchLastSection(true);
    m_teachersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    // Auto-load slots when teacher is clicked
    connect(m_teachersTable, &QTableWidget::cellClicked, [this](int row, int column) {
        int teacherId = m_teachersTable->item(row, 0)->text().toInt();
        m_teacherIdSpin->setValue(teacherId);
        onViewSlots();
    });
    tgLayout->addWidget(m_teachersTable);
    teachersLayout->addWidget(teachersGroup);

    // View slots
    QGroupBox *slotsGroup = new QGroupBox("Xem Slot của Giảng viên");
    QVBoxLayout *sgLayout = new QVBoxLayout(slotsGroup);
    
    QHBoxLayout *slotInputLayout = new QHBoxLayout();
    slotInputLayout->addWidget(new QLabel("ID Giảng viên:"));
    m_teacherIdSpin = new QSpinBox();
    m_teacherIdSpin->setRange(1, 9999);
    slotInputLayout->addWidget(m_teacherIdSpin);
    QPushButton *viewSlotsBtn = new QPushButton("Xem slots");
    connect(viewSlotsBtn, &QPushButton::clicked, this, &StudentWidget::onViewSlots);
    slotInputLayout->addWidget(viewSlotsBtn);
    slotInputLayout->addStretch();
    sgLayout->addLayout(slotInputLayout);
    
    // Filter and date range controls
    QHBoxLayout *filterLayout = new QHBoxLayout();
    
    QCheckBox *availableOnlyCheck = new QCheckBox("Chỉ hiển thị slots trống");
    availableOnlyCheck->setChecked(false);
    filterLayout->addWidget(availableOnlyCheck);
    
    filterLayout->addWidget(new QLabel("   |   Từ ngày:"));
    QDateEdit *startDateEdit = new QDateEdit(QDate::currentDate());
    startDateEdit->setCalendarPopup(true);
    startDateEdit->setDisplayFormat("dd/MM/yyyy");
    filterLayout->addWidget(startDateEdit);
    
    filterLayout->addWidget(new QLabel("Đến:"));
    QDateEdit *endDateEdit = new QDateEdit(QDate::currentDate().addDays(30));
    endDateEdit->setCalendarPopup(true);
    endDateEdit->setDisplayFormat("dd/MM/yyyy");
    filterLayout->addWidget(endDateEdit);
    
    filterLayout->addStretch();
    sgLayout->addLayout(filterLayout);
    
    // Store references for filtering
    m_availableOnlyCheck = availableOnlyCheck;
    m_startDateEdit = startDateEdit;
    m_endDateEdit = endDateEdit;
    
    // Connect filter/date changes to re-display
    connect(availableOnlyCheck, &QCheckBox::stateChanged, this, &StudentWidget::onViewSlots);
    connect(startDateEdit, &QDateEdit::dateChanged, this, &StudentWidget::onViewSlots);
    connect(endDateEdit, &QDateEdit::dateChanged, this, &StudentWidget::onViewSlots);
    
    m_slotsTable = new QTableWidget();
    m_slotsTable->setColumnCount(8);
    m_slotsTable->setHorizontalHeaderLabels({"ID", "Ngày", "Bắt đầu", "Kết thúc", "Loại", "Max", "Trạng thái", "Action"});
    m_slotsTable->horizontalHeader()->setStretchLastSection(true);
    m_slotsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    sgLayout->addWidget(m_slotsTable);
    
    // Booking type selection
    QGroupBox *bookingTypeBox = new QGroupBox("Loại đặt lịch");
    QVBoxLayout *btLayout = new QVBoxLayout(bookingTypeBox);
    
    QRadioButton *individualRadio = new QRadioButton("Cá nhân");
    individualRadio->setChecked(true);
    btLayout->addWidget(individualRadio);
    
    QHBoxLayout *groupRadioLayout = new QHBoxLayout();
    QRadioButton *groupRadio = new QRadioButton("Nhóm:");
    groupRadioLayout->addWidget(groupRadio);
    
    QComboBox *groupCombo = new QComboBox();
    groupCombo->setEnabled(false);
    groupRadioLayout->addWidget(groupCombo);
    groupRadioLayout->addStretch();
    btLayout->addLayout(groupRadioLayout);
    
    // Enable/disable group combo based on radio selection
    connect(individualRadio, &QRadioButton::toggled, [groupCombo](bool checked) {
        groupCombo->setEnabled(!checked);
    });
    
    sgLayout->addWidget(bookingTypeBox);
    
    QPushButton *bookBtn = new QPushButton("Đặt lịch slot đã chọn");
    bookBtn->setStyleSheet("background-color: #4CAF50; color: white; padding: 8px;");
    
    // Pass groupCombo and groupRadio to booking slot via lambda
    connect(bookBtn, &QPushButton::clicked, [this, groupRadio, groupCombo]() {
        onBookMeeting(groupRadio->isChecked(), groupCombo->currentData().toInt());
    });
    sgLayout->addWidget(bookBtn);
    
    // Store groupCombo reference for loading groups
    m_groupComboForBooking = groupCombo;
    
    teachersLayout->addWidget(slotsGroup);
    m_tabWidget->addTab(teachersTab, "Giảng viên");

    // === TAB 2: My Meetings ===
    QWidget *meetingsTab = new QWidget();
    QVBoxLayout *meetingsLayout = new QVBoxLayout(meetingsTab);

    QHBoxLayout *meetingsActions = new QHBoxLayout();
    QPushButton *refreshMeetingsBtn = new QPushButton("Tải lịch hẹn");
    connect(refreshMeetingsBtn, &QPushButton::clicked, this, &StudentWidget::onViewMeetings);
    meetingsActions->addWidget(refreshMeetingsBtn);
    meetingsActions->addStretch();
    meetingsLayout->addLayout(meetingsActions);
    
    m_meetingsTable = new QTableWidget();
    m_meetingsTable->setColumnCount(6);
    m_meetingsTable->setHorizontalHeaderLabels({"ID", "Ngày", "Bắt đầu", "Kết thúc", "Loại", "Trạng thái"});
    m_meetingsTable->horizontalHeader()->setStretchLastSection(true);
    m_meetingsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    meetingsLayout->addWidget(m_meetingsTable);
    
    QHBoxLayout *meetingActions = new QHBoxLayout();
    m_meetingIdSpin = new QSpinBox();
    m_meetingIdSpin->setRange(1, 9999);
    meetingActions->addWidget(new QLabel("Meeting ID:"));
    meetingActions->addWidget(m_meetingIdSpin);
    
    QPushButton *cancelBtn = new QPushButton("Hủy lịch");
    cancelBtn->setStyleSheet("background-color: #f44336; color: white;");
    connect(cancelBtn, &QPushButton::clicked, this, &StudentWidget::onCancelMeeting);
    meetingActions->addWidget(cancelBtn);
    
    QPushButton *viewMinutesBtn = new QPushButton("Xem biên bản");
    connect(viewMinutesBtn, &QPushButton::clicked, this, &StudentWidget::onViewMinutes);
    meetingActions->addWidget(viewMinutesBtn);
    meetingActions->addStretch();
    meetingsLayout->addLayout(meetingActions);
    
    // === History Section (moved into Meetings tab) ===
    QGroupBox *historyGroup = new QGroupBox("Lịch sử cuộc họp");
    QVBoxLayout *historyLayout = new QVBoxLayout(historyGroup);
    
    QPushButton *refreshHistoryBtn = new QPushButton("Tải lịch sử");
    connect(refreshHistoryBtn, &QPushButton::clicked, this, &StudentWidget::onViewHistory);
    historyLayout->addWidget(refreshHistoryBtn);
    
    m_historyTable = new QTableWidget();
    m_historyTable->setColumnCount(7);
    m_historyTable->setHorizontalHeaderLabels({"ID", "Ngày", "Thời gian", "Giảng viên", "Loại", "Biên bản", "Action"});
    m_historyTable->horizontalHeader()->setStretchLastSection(true);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    historyLayout->addWidget(m_historyTable);
    
    meetingsLayout->addWidget(historyGroup);
    m_tabWidget->addTab(meetingsTab, "Lịch hẹn");

    // === TAB 3: Groups ===
    QWidget *groupsTab = new QWidget();
    QVBoxLayout *groupsLayout = new QVBoxLayout(groupsTab);

    QPushButton *refreshGroupsBtn = new QPushButton("Tải danh sách nhóm");
    connect(refreshGroupsBtn, &QPushButton::clicked, this, &StudentWidget::onViewGroups);
    groupsLayout->addWidget(refreshGroupsBtn);

    // Create sub-tabs for better organization
    QTabWidget *groupTabWidget = new QTabWidget();
    
    // --- SUB-TAB 1: My Groups ---
    QWidget *myGroupsTab = new QWidget();
    QVBoxLayout *myGroupsLayout = new QVBoxLayout(myGroupsTab);
    
    m_groupsTable = new QTableWidget();
    m_groupsTable->setColumnCount(4);
    m_groupsTable->setHorizontalHeaderLabels({"ID", "Tên nhóm", "Số TV", "Vai trò"});
    m_groupsTable->horizontalHeader()->setStretchLastSection(true);
    m_groupsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    myGroupsLayout->addWidget(m_groupsTable);
    
    groupTabWidget->addTab(myGroupsTab, "Nhóm của tôi");
    
    // --- SUB-TAB 2: Other Groups ---
    QWidget *otherGroupsTab = new QWidget();
    QVBoxLayout *otherGroupsLayout = new QVBoxLayout(otherGroupsTab);
    
    m_otherGroupsTable = new QTableWidget();
    m_otherGroupsTable->setColumnCount(3);
    m_otherGroupsTable->setHorizontalHeaderLabels({"ID", "Tên nhóm", "Số TV"});
    m_otherGroupsTable->horizontalHeader()->setStretchLastSection(true);
    m_otherGroupsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    otherGroupsLayout->addWidget(m_otherGroupsTable);
    
    QHBoxLayout *joinLayout = new QHBoxLayout();
    joinLayout->addWidget(new QLabel("Nhập Group ID để xin vào:"));
    m_groupIdSpin = new QSpinBox();
    m_groupIdSpin->setRange(1, 9999);
    joinLayout->addWidget(m_groupIdSpin);
    QPushButton *joinBtn = new QPushButton("Gửi yêu cầu tham gia");
    joinBtn->setStyleSheet("background-color: #2196F3; color: white;");
    connect(joinBtn, &QPushButton::clicked, this, &StudentWidget::onRequestJoinGroup);
    joinLayout->addWidget(joinBtn);
    joinLayout->addStretch();
    otherGroupsLayout->addLayout(joinLayout);
    
    groupTabWidget->addTab(otherGroupsTab, "Nhóm khác");
    
    // --- SUB-TAB 3: Join Requests (for admin groups) ---
    QWidget *requestsTab = new QWidget();
    QVBoxLayout *requestsLayout = new QVBoxLayout(requestsTab);
    
    QHBoxLayout *viewReqLayout = new QHBoxLayout();
    viewReqLayout->addWidget(new QLabel("Group ID (nhóm bạn là Admin):"));
    QSpinBox *reqGroupIdSpin = new QSpinBox();
    reqGroupIdSpin->setRange(1, 9999);
    viewReqLayout->addWidget(reqGroupIdSpin);
    QPushButton *viewReqBtn = new QPushButton("Xem yêu cầu");
    connect(viewReqBtn, &QPushButton::clicked, [this, reqGroupIdSpin]() {
        m_groupIdSpin->setValue(reqGroupIdSpin->value());
        onViewJoinRequests();
    });
    viewReqLayout->addWidget(viewReqBtn);
    viewReqLayout->addStretch();
    requestsLayout->addLayout(viewReqLayout);
    
    m_requestsTable = new QTableWidget();
    m_requestsTable->setColumnCount(4);
    m_requestsTable->setHorizontalHeaderLabels({"Request ID", "User ID", "Họ tên", "Ghi chú"});
    m_requestsTable->horizontalHeader()->setStretchLastSection(true);
    m_requestsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    requestsLayout->addWidget(m_requestsTable);
    
    QHBoxLayout *approveLayout = new QHBoxLayout();
    approveLayout->addWidget(new QLabel("Request ID:"));
    m_requestIdSpin = new QSpinBox();
    m_requestIdSpin->setRange(1, 9999);
    approveLayout->addWidget(m_requestIdSpin);
    QPushButton *approveBtn = new QPushButton("Duyệt");
    approveBtn->setStyleSheet("background-color: #4CAF50; color: white;");
    connect(approveBtn, &QPushButton::clicked, this, &StudentWidget::onApproveRequest);
    approveLayout->addWidget(approveBtn);
    QPushButton *rejectBtn = new QPushButton("Từ chối");
    rejectBtn->setStyleSheet("background-color: #f44336; color: white;");
    connect(rejectBtn, &QPushButton::clicked, this, &StudentWidget::onRejectRequest);
    approveLayout->addWidget(rejectBtn);
    approveLayout->addStretch();
    requestsLayout->addLayout(approveLayout);
    
    groupTabWidget->addTab(requestsTab, "Quản lý yêu cầu");
    
    // --- SUB-TAB 4: Create Group ---
    QWidget *createGroupTab = new QWidget();
    QVBoxLayout *createGroupLayout = new QVBoxLayout(createGroupTab);
    
    createGroupLayout->addWidget(new QLabel("Tạo nhóm mới:"));
    QHBoxLayout *createLayout = new QHBoxLayout();
    createLayout->addWidget(new QLabel("Tên nhóm:"));
    m_groupNameEdit = new QLineEdit();
    createLayout->addWidget(m_groupNameEdit);
    QPushButton *createGroupBtn = new QPushButton("Tạo nhóm");
    createGroupBtn->setStyleSheet("background-color: #4CAF50; color: white;");
    connect(createGroupBtn, &QPushButton::clicked, this, &StudentWidget::onCreateGroup);
    createLayout->addWidget(createGroupBtn);
    createGroupLayout->addLayout(createLayout);
    createGroupLayout->addStretch();
    
    groupTabWidget->addTab(createGroupTab, "Tạo nhóm");
    
    groupsLayout->addWidget(groupTabWidget);
    m_tabWidget->addTab(groupsTab, "Nhóm");

    mainLayout->addWidget(m_tabWidget);
}

void StudentWidget::refresh() {
    onViewMeetings();
    onViewHistory();
}

void StudentWidget::clearTable(QTableWidget *table) {
    table->setRowCount(0);
}

void StudentWidget::loadAdminGroupsIntoCombo() {
    if (!m_groupComboForBooking) return;
    
    m_groupComboForBooking->clear();
    m_groupComboForBooking->addItem("Chọn nhóm...", 0);
    
    QString cmd = QString("VIEW_GROUPS|token=%1\r\n").arg(m_networkManager->getToken());
    QString response = m_networkManager->sendRequest(cmd);
    
    QString groups = NetworkManager::getValue(response, "groups");
    if (!groups.isEmpty()) {
        QStringList list = groups.split('#');
        for (const QString &g : list) {
            QStringList parts = g.split(',');
            // Format: group_id,group_name,member_count,is_member,role
            if (parts.size() >= 5) {
                int is_member = parts[3].toInt();
                int role = parts[4].toInt();
                
                // Only add groups where user is admin (role=1)
                if (is_member == 1 && role == 1) {
                    QString groupName = parts[1];
                    int groupId = parts[0].toInt();
                    QString memberCount = parts[2];
                    m_groupComboForBooking->addItem(
                        QString("%1 (%2 thành viên)").arg(groupName, memberCount), 
                        groupId
                    );
                }
            }
        }
    }
}

void StudentWidget::onViewTeachers() {
    QString cmd = QString("VIEW_TEACHERS|token=%1\r\n").arg(m_networkManager->getToken());
    QString response = m_networkManager->sendRequest(cmd);
    
    clearTable(m_teachersTable);
    QString teachers = NetworkManager::getValue(response, "teachers");
    
    if (!teachers.isEmpty()) {
        QStringList list = teachers.split('#');
        for (const QString &t : list) {
            QStringList parts = t.split(',');
            if (parts.size() >= 3) {
                int row = m_teachersTable->rowCount();
                m_teachersTable->insertRow(row);
                m_teachersTable->setItem(row, 0, new QTableWidgetItem(parts[0]));
                m_teachersTable->setItem(row, 1, new QTableWidgetItem(parts[1]));
                m_teachersTable->setItem(row, 2, new QTableWidgetItem(parts[2]));
            }
        }
    }
}

void StudentWidget::onViewSlots() {
    int teacherId = m_teacherIdSpin->value();
    QString cmd = QString("VIEW_SLOTS|teacher_id=%1\r\n").arg(teacherId);
    QString response = m_networkManager->sendRequest(cmd);
    
    clearTable(m_slotsTable);
    QString slotsData = NetworkManager::getValue(response, "slots");
    
    if (!slotsData.isEmpty()) {
        QStringList list = slotsData.split('#');
        
        // Get filter settings
        bool availableOnly = m_availableOnlyCheck->isChecked();
        QDate startDate = m_startDateEdit->date();
        QDate endDate = m_endDateEdit->date();
        
        // Collect slots with date for sorting
        QVector<QStringList> slotsList;
        
        for (const QString &s : list) {
            QStringList parts = s.split(',');
            if (parts.size() >= 7) {
                QString status = parts[6];
                QDate slotDate = QDate::fromString(parts[1], "yyyy-MM-dd");
                
                // Apply filters
                if (availableOnly && status != "AVAILABLE") {
                    continue;
                }
                
                if (slotDate < startDate || slotDate > endDate) {
                    continue;
                }
                
                slotsList.append(parts);
            }
        }
        
        // Sort by date then time
        std::sort(slotsList.begin(), slotsList.end(), [](const QStringList &a, const QStringList &b) {
            if (a[1] != b[1]) {  // date
                return a[1] < b[1];
            }
            return a[2] < b[2];  // start_time
        });
        
        // Display sorted and filtered slots
        for (const QStringList &parts : slotsList) {
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
                statusItem->setForeground(QColor("#4CAF50"));
            } else {
                statusItem->setForeground(QColor("#999"));
            }
            m_slotsTable->setItem(row, 6, statusItem);
            
            // Action button
            if (status == "AVAILABLE") {
                QPushButton *bookBtn = new QPushButton("Đặt lịch");
                bookBtn->setStyleSheet("background-color: #4CAF50; color: white; padding: 4px;");
                
                connect(bookBtn, &QPushButton::clicked, [this, slotId, teacherId]() {
                    // Auto-select the row
                    for (int i = 0; i < m_slotsTable->rowCount(); ++i) {
                        if (m_slotsTable->item(i, 0)->text().toInt() == slotId) {
                            m_slotsTable->selectRow(i);
                            break;
                        }
                    }
                    
                    // Auto-fill teacher ID
                    m_teacherIdSpin->setValue(teacherId);
                    
                    // Trigger booking based on current selection
                    // Get the current booking type from the radio buttons in the UI
                    // For simplicity, assume we have access to the parent widget structure
                    // Or we can show a quick dialog
                    
                    QMessageBox msgBox(this);
                    msgBox.setWindowTitle("Chọn loại đặt lịch");
                    msgBox.setText(QString("Đặt lịch slot #%1").arg(slotId));
                    QPushButton *individualBtn = msgBox.addButton("Cá nhân", QMessageBox::ActionRole);
                    QPushButton *groupBtn = msgBox.addButton("Nhóm", QMessageBox::ActionRole);
                    msgBox.addButton(QMessageBox::Cancel);
                    
                    msgBox.exec();
                    
                    if (msgBox.clickedButton() == individualBtn) {
                        onBookMeeting(false, 0);  // Individual booking
                    } else if (msgBox.clickedButton() == groupBtn) {
                        if (m_groupComboForBooking && m_groupComboForBooking->count() > 0) {
                            int groupId = m_groupComboForBooking->currentData().toInt();
                            onBookMeeting(true, groupId);  // Group booking
                        } else {
                            QMessageBox::warning(this, "Lỗi", "Bạn chưa là admin của nhóm nào!");
                        }
                    }
                });
                m_slotsTable->setCellWidget(row, 7, bookBtn);
            } else {
                QLabel *label = new QLabel("  —  ");
                label->setAlignment(Qt::AlignCenter);
                label->setStyleSheet("color: #999;");
                m_slotsTable->setCellWidget(row, 7, label);
            }
        }
        
        // Load admin groups into combo for booking
        loadAdminGroupsIntoCombo();
    }
}

void StudentWidget::onBookMeeting(bool isGroupBooking, int groupId) {
    int currentRow = m_slotsTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "Lỗi", "Vui lòng chọn một slot!");
        return;
    }
    
    QString slotId = m_slotsTable->item(currentRow, 0)->text();
    int teacherId = m_teacherIdSpin->value();
    
    QString cmd;
    if (isGroupBooking) {
        if (groupId == 0) {
            QMessageBox::warning(this, "Lỗi", "Vui lòng chọn nhóm để đặt lịch!");
            return;
        }
        cmd = QString("BOOK_MEETING_GROUP|token=%1;teacher_id=%2;slot_id=%3;group_id=%4\r\n")
                          .arg(m_networkManager->getToken())
                          .arg(teacherId)
                          .arg(slotId)
                          .arg(groupId);
    } else {
        cmd = QString("BOOK_MEETING_INDIV|token=%1;teacher_id=%2;slot_id=%3\r\n")
                          .arg(m_networkManager->getToken())
                          .arg(teacherId)
                          .arg(slotId);
    }
    
    QString response = m_networkManager->sendRequest(cmd);
    int status = NetworkManager::getStatusCode(response);
    if (status == 201) {
        QMessageBox::information(this, "Thành công", "Đã đặt lịch thành công!");
        onViewSlots(); // Refresh
    } else if (status == 409) {
        // Parse the actual error message from server
        QString msg = NetworkManager::getValue(response, "msg");
        QString errorMsg = "Không thể đặt lịch: ";
        
        if (msg.contains("Time_conflict")) {
            errorMsg += "Bạn đã có lịch họp trùng giờ!";
            QString conflictMeeting = NetworkManager::getValue(response, "conflicting_meeting");
            if (!conflictMeeting.isEmpty()) {
                errorMsg += QString("\n(Meeting #%1)").arg(conflictMeeting);
            }
        } else if (msg.contains("Slot_already_booked")) {
            errorMsg += "Slot đã được người khác đặt rồi!";
        } else if (msg.contains("Group_size_exceeded")) {
            errorMsg += "Nhóm vượt quá số người tối đa của slot!";
        } else {
            errorMsg += msg.replace('_', ' ');
        }
        
        QMessageBox::warning(this, "Lỗi", errorMsg);
    } else if (status == 403) {
        QMessageBox::warning(this, "Lỗi", "Bạn không phải admin của nhóm!");
    } else if (status == 400) {
        QString msg = NetworkManager::getValue(response, "msg");
        if (msg.contains("Group_too_large")) {
            QString memberCount = NetworkManager::getValue(response, "member_count");
            QString maxSize = NetworkManager::getValue(response, "max_size");
            QMessageBox::warning(this, "Lỗi", 
                QString("Nhóm quá lớn!\nSố thành viên: %1\nSlot chỉ cho phép: %2")
                    .arg(memberCount, maxSize));
        } else {
            QMessageBox::warning(this, "Lỗi", "Đặt lịch thất bại!");
        }
    } else {
        QMessageBox::warning(this, "Lỗi", "Đặt lịch thất bại!");
    }
}

void StudentWidget::onViewMeetings() {
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
                m_meetingsTable->setItem(row, 0, new QTableWidgetItem(parts[0])); // ID
                m_meetingsTable->setItem(row, 1, new QTableWidgetItem(parts[1])); // Date
                m_meetingsTable->setItem(row, 2, new QTableWidgetItem(parts[2])); // Start
                m_meetingsTable->setItem(row, 3, new QTableWidgetItem(parts[3])); // End
                m_meetingsTable->setItem(row, 4, new QTableWidgetItem(parts[6])); // Type
                m_meetingsTable->setItem(row, 5, new QTableWidgetItem(parts[7])); // Status
            }
        }
    }
}

void StudentWidget::onCancelMeeting() {
    int meetingId = m_meetingIdSpin->value();
    
    bool ok;
    QString reason = QInputDialog::getText(this, "Hủy lịch", "Lý do hủy:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    
    QString cmd = QString("CANCEL_MEETING|token=%1;meeting_id=%2;reason=%3\r\n")
                      .arg(m_networkManager->getToken())
                      .arg(meetingId)
                      .arg(reason);
    QString response = m_networkManager->sendRequest(cmd);
    
    int status = NetworkManager::getStatusCode(response);
    if (status == 200) {
        QMessageBox::information(this, "Thành công", "Đã hủy lịch hẹn!");
        onViewMeetings();
    } else {
        QMessageBox::warning(this, "Lỗi", "Không thể hủy lịch hẹn!");
    }
}

void StudentWidget::onViewHistory() {
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
                    m_historyTable->setItem(row, 3, new QTableWidgetItem(parts[4]));  // Teacher name
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

void StudentWidget::onViewMinutes() {
    int meetingId = m_meetingIdSpin->value();
    
    QString cmd = QString("VIEW_MINUTES|token=%1;meeting_id=%2\r\n")
                      .arg(m_networkManager->getToken())
                      .arg(meetingId);
    QString response = m_networkManager->sendRequest(cmd);
    
    QString content = NetworkManager::getValue(response, "content");
    QString minuteId = NetworkManager::getValue(response, "minute_id");
    
    if (minuteId == "0" || content.isEmpty()) {
        QMessageBox::information(this, "Biên bản", "Chưa có biên bản cho cuộc họp này.");
    } else {
        QMessageBox::information(this, "Biên bản cuộc họp", content);
    }
}

void StudentWidget::onViewGroups() {
    QString cmd = QString("VIEW_GROUPS|token=%1\r\n").arg(m_networkManager->getToken());
    QString response = m_networkManager->sendRequest(cmd);
    
    clearTable(m_groupsTable);
    clearTable(m_otherGroupsTable);
    QString groups = NetworkManager::getValue(response, "groups");
    
    if (!groups.isEmpty()) {
        QStringList list = groups.split('#');
        for (const QString &g : list) {
            QStringList parts = g.split(',');
            // Format: group_id,group_name,member_count,is_member,role
            if (parts.size() >= 5) {
                int is_member = parts[3].toInt();
                int role = parts[4].toInt();
                
                if (is_member == 1) {
                    // User is member of this group - add to joined table
                    int row = m_groupsTable->rowCount();
                    m_groupsTable->insertRow(row);
                    m_groupsTable->setItem(row, 0, new QTableWidgetItem(parts[0])); // ID
                    m_groupsTable->setItem(row, 1, new QTableWidgetItem(parts[1])); // Name
                    m_groupsTable->setItem(row, 2, new QTableWidgetItem(parts[2])); // Member count
                    m_groupsTable->setItem(row, 3, new QTableWidgetItem(role == 1 ? "Admin" : "Member"));
                } else {
                    // User not in this group - add to other table
                    int row = m_otherGroupsTable->rowCount();
                    m_otherGroupsTable->insertRow(row);
                    m_otherGroupsTable->setItem(row, 0, new QTableWidgetItem(parts[0])); // ID
                    m_otherGroupsTable->setItem(row, 1, new QTableWidgetItem(parts[1])); // Name
                    m_otherGroupsTable->setItem(row, 2, new QTableWidgetItem(parts[2])); // Member count
                }
            }
        }
    }
}

void StudentWidget::onCreateGroup() {
    QString groupName = m_groupNameEdit->text().trimmed();
    if (groupName.isEmpty()) {
        QMessageBox::warning(this, "Lỗi", "Vui lòng nhập tên nhóm!");
        return;
    }
    
    QString cmd = QString("CREATE_GROUP|token=%1;group_name=%2\r\n")
                      .arg(m_networkManager->getToken(), groupName);
    QString response = m_networkManager->sendRequest(cmd);
    
    int status = NetworkManager::getStatusCode(response);
    if (status == 201) {
        QMessageBox::information(this, "Thành công", "Tạo nhóm thành công!");
        m_groupNameEdit->clear();
        onViewGroups();
    } else {
        QMessageBox::warning(this, "Lỗi", "Tạo nhóm thất bại!");
    }
}

void StudentWidget::onRequestJoinGroup() {
    int groupId = m_groupIdSpin->value();
    
    bool ok;
    QString note = QInputDialog::getText(this, "Yêu cầu vào nhóm", "Ghi chú:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    
    QString cmd = QString("REQUEST_JOIN_GROUP|token=%1;group_id=%2;note=%3\r\n")
                      .arg(m_networkManager->getToken())
                      .arg(groupId)
                      .arg(note);
    QString response = m_networkManager->sendRequest(cmd);
    
    int status = NetworkManager::getStatusCode(response);
    if (status == 202) {
        QMessageBox::information(this, "Thành công", "Đã gửi yêu cầu vào nhóm!");
    } else if (status == 409) {
        QMessageBox::warning(this, "Lỗi", "Bạn đã là thành viên hoặc đã gửi yêu cầu!");
    } else {
        QMessageBox::warning(this, "Lỗi", "Gửi yêu cầu thất bại!");
    }
}

void StudentWidget::onViewJoinRequests() {
    int groupId = m_groupIdSpin->value();
    
    QString cmd = QString("VIEW_JOIN_REQUESTS|token=%1;group_id=%2\r\n")
                      .arg(m_networkManager->getToken())
                      .arg(groupId);
    QString response = m_networkManager->sendRequest(cmd);
    
    clearTable(m_requestsTable);
    int status = NetworkManager::getStatusCode(response);
    
    if (status == 403) {
        QMessageBox::warning(this, "Lỗi", "Bạn không phải admin của nhóm này!");
        return;
    }
    
    QString requests = NetworkManager::getValue(response, "requests");
    
    if (!requests.isEmpty()) {
        QStringList list = requests.split('#');
        for (const QString &r : list) {
            QStringList parts = r.split(',');
            if (parts.size() >= 4) {
                int row = m_requestsTable->rowCount();
                m_requestsTable->insertRow(row);
                m_requestsTable->setItem(row, 0, new QTableWidgetItem(parts[0])); // Request ID
                m_requestsTable->setItem(row, 1, new QTableWidgetItem(parts[1])); // User ID
                m_requestsTable->setItem(row, 2, new QTableWidgetItem(parts[2])); // Full name
                m_requestsTable->setItem(row, 3, new QTableWidgetItem(parts[3])); // Note
            }
        }
    }
}

void StudentWidget::onApproveRequest() {
    int requestId = m_requestIdSpin->value();
    
    QString cmd = QString("APPROVE_JOIN_REQUEST|token=%1;request_id=%2\r\n")
                      .arg(m_networkManager->getToken())
                      .arg(requestId);
    QString response = m_networkManager->sendRequest(cmd);
    
    int status = NetworkManager::getStatusCode(response);
    if (status == 200) {
        QMessageBox::information(this, "Thành công", "Đã duyệt yêu cầu!");
        onViewJoinRequests();
    } else {
        QMessageBox::warning(this, "Lỗi", "Duyệt yêu cầu thất bại!");
    }
}

void StudentWidget::onRejectRequest() {
    int requestId = m_requestIdSpin->value();
    
    QString cmd = QString("REJECT_JOIN_REQUEST|token=%1;request_id=%2\r\n")
                      .arg(m_networkManager->getToken())
                      .arg(requestId);
    QString response = m_networkManager->sendRequest(cmd);
    
    int status = NetworkManager::getStatusCode(response);
    if (status == 200) {
        QMessageBox::information(this, "Thành công", "Đã từ chối yêu cầu!");
        onViewJoinRequests();
    } else {
        QMessageBox::warning(this, "Lỗi", "Từ chối yêu cầu thất bại!");
    }
}

void StudentWidget::onLogout() {
    emit logoutRequested();
}
