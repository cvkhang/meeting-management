#ifndef TEACHERWIDGET_H
#define TEACHERWIDGET_H

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
#include <QDateEdit>
#include <QTimeEdit>
#include "networkmanager.h"

class TeacherWidget : public QWidget {
    Q_OBJECT

public:
    explicit TeacherWidget(NetworkManager *networkManager, QWidget *parent = nullptr);
    void refresh();

signals:
    void logoutRequested();

private slots:
    void onViewMySlots();
    void onCreateSlot();
    void onEditSlot();
    void onDeleteSlot();
    void onViewMeetings();
    void onViewMeetingDetail();
    void onWriteMinutes();
    void onViewMinutes();
    void onUpdateMinutes();
    void onCompleteMeeting();
    void onViewHistory();
    void onLogout();

private:
    void setupUi();
    void clearTable(QTableWidget *table);

    NetworkManager *m_networkManager;
    QTabWidget *m_tabWidget;
    
    // Slots tab
    QTableWidget *m_slotsTable;
    QDateEdit *m_dateEdit;
    QTimeEdit *m_startTimeEdit;
    QTimeEdit *m_endTimeEdit;
    QComboBox *m_slotTypeCombo;
    QSpinBox *m_maxGroupSpin;
    QSpinBox *m_slotIdSpin;
    
    // Meetings tab
    QTableWidget *m_meetingsTable;
    QSpinBox *m_meetingIdSpin;
    QTextEdit *m_minutesEdit;
    int m_currentMinuteId;
    
    // History tab
    QTableWidget *m_historyTable;
};

#endif // TEACHERWIDGET_H
