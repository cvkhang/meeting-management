-- Users table
CREATE TABLE IF NOT EXISTS users (
    user_id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password TEXT NOT NULL,
    full_name TEXT NOT NULL,
    role TEXT CHECK(role IN ('STUDENT', 'TEACHER')) NOT NULL
);
-- Groups table
CREATE TABLE IF NOT EXISTS groups (
    group_id INTEGER PRIMARY KEY AUTOINCREMENT,
    group_name TEXT NOT NULL,
    created_by INTEGER,
    FOREIGN KEY(created_by) REFERENCES users(user_id)
);
-- Group Members table
CREATE TABLE IF NOT EXISTS group_members (
    group_id INTEGER,
    user_id INTEGER,
    role INTEGER DEFAULT 0,
    -- 1: Admin, 0: Member
    PRIMARY KEY (group_id, user_id),
    FOREIGN KEY(group_id) REFERENCES groups(group_id),
    FOREIGN KEY(user_id) REFERENCES users(user_id)
);
-- Join Requests table
CREATE TABLE IF NOT EXISTS join_requests (
    request_id INTEGER PRIMARY KEY AUTOINCREMENT,
    group_id INTEGER,
    user_id INTEGER,
    note TEXT,
    status TEXT DEFAULT 'PENDING',
    -- PENDING, APPROVED, REJECTED
    FOREIGN KEY(group_id) REFERENCES groups(group_id),
    FOREIGN KEY(user_id) REFERENCES users(user_id)
);
-- Slots table (Teacher's available slots)
CREATE TABLE IF NOT EXISTS slots (
    slot_id INTEGER PRIMARY KEY AUTOINCREMENT,
    teacher_id INTEGER,
    date TEXT NOT NULL,
    -- YYYY-MM-DD
    start_time TEXT NOT NULL,
    -- HH:MM
    end_time TEXT NOT NULL,
    -- HH:MM
    slot_type TEXT CHECK(slot_type IN ('INDIVIDUAL', 'GROUP', 'BOTH')) NOT NULL,
    max_group_size INTEGER DEFAULT 1,
    FOREIGN KEY(teacher_id) REFERENCES users(user_id)
);
-- Meetings table
CREATE TABLE IF NOT EXISTS meetings (
    meeting_id INTEGER PRIMARY KEY AUTOINCREMENT,
    slot_id INTEGER,
    teacher_id INTEGER,
    student_id INTEGER,
    -- If individual meeting
    group_id INTEGER,
    -- If group meeting
    meeting_type TEXT CHECK(meeting_type IN ('INDIVIDUAL', 'GROUP')),
    status TEXT DEFAULT 'BOOKED',
    -- BOOKED, CANCELLED, DONE
    FOREIGN KEY(slot_id) REFERENCES slots(slot_id),
    FOREIGN KEY(teacher_id) REFERENCES users(user_id),
    FOREIGN KEY(student_id) REFERENCES users(user_id),
    FOREIGN KEY(group_id) REFERENCES groups(group_id)
);
-- Meeting Minutes table
CREATE TABLE IF NOT EXISTS meeting_minutes (
    minute_id INTEGER PRIMARY KEY AUTOINCREMENT,
    meeting_id INTEGER UNIQUE,
    content TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(meeting_id) REFERENCES meetings(meeting_id)
);
-- Pending Notifications table (for offline users)
CREATE TABLE IF NOT EXISTS pending_notifications (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER,
    type TEXT NOT NULL,
    payload TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(user_id) REFERENCES users(user_id)
);