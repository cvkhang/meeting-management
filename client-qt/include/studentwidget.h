#ifndef STUDENTWIDGET_H
#define STUDENTWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QGroupBox>
#include <QSpinBox>
#include <QRadioButton>
#include <QDateEdit>
#include <QCheckBox>
#include "networkmanager.h"

class StudentWidget : public QWidget {
    Q_OBJECT

public:
    explicit StudentWidget(NetworkManager *networkManager, QWidget *parent = nullptr);
    void refresh();

signals:
    void logoutRequested();

private slots:
    void onViewTeachers();
    void onViewSlots();
    void onBookMeeting(bool isGroupBooking = false, int groupId = 0);
    void onViewMeetings();
    void onCancelMeeting();
    void onViewHistory();
    void onViewMinutes();
    void onViewGroups();
    void onCreateGroup();
    void onRequestJoinGroup();
    void onViewJoinRequests();
    void onApproveRequest();
    void onRejectRequest();
    void onLogout();

private:
    void setupUi();
    void clearTable(QTableWidget *table);
    void loadAdminGroupsIntoCombo();

    NetworkManager *m_networkManager;
    QTabWidget *m_tabWidget;
    
    // Teachers tab
    QTableWidget *m_teachersTable;
    QSpinBox *m_teacherIdSpin;
    QTableWidget *m_slotsTable;
    
    // Meetings tab
    QTableWidget *m_meetingsTable;
    QSpinBox *m_meetingIdSpin;
    
    // Groups tab
    QTableWidget *m_groupsTable;       // Groups user joined
    QTableWidget *m_otherGroupsTable;  // Groups user hasn't joined
    QTableWidget *m_requestsTable;     // Join requests for admin groups
    QLineEdit *m_groupNameEdit;
    QSpinBox *m_groupIdSpin;
    QSpinBox *m_requestIdSpin;
    
    // History tab
    QTableWidget *m_historyTable;
    
    // Helper for group booking
    QComboBox *m_groupComboForBooking;
    
    // Filter controls
    QCheckBox *m_availableOnlyCheck;
    QDateEdit *m_startDateEdit;
    QDateEdit *m_endDateEdit;
};

#endif // STUDENTWIDGET_H
