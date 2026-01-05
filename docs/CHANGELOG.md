# Changelog - Meeting Management System

## Version 2.0 - 2026-01-05

### 🎯 Major Features Implemented

#### Server-Side Validations

1. **Slot Type Constraints**
   - AUTO-ENFORCE `max_group_size=1` for INDIVIDUAL slots
   - Default `max_group_size=5` for GROUP/BOTH slots
   - Prevents invalid capacity configurations

2. **Edit/Delete Protection**
   - Slots with active bookings (status='BOOKED') cannot be modified or deleted
   - Returns `403|msg=Cannot_modify_booked_slot` error
   - Protects data integrity and student bookings

3. **Group Existence Validation**
   - Validates group_id exists before accepting join requests
   - Returns `404|msg=Group_not_found` for invalid groups
   - Prevents orphaned requests

4. **Group Meeting Capacity Validation**
   - Checks group member count vs slot max_group_size
   - Returns detailed error: `400|msg=Group_too_large;member_count=X;max_size=Y`
   - Only group admins can book meetings for groups

#### Client-Side UI Improvements

1. **TeacherWidget Enhancements**
   - **View Minutes Button**: Load and display existing meeting minutes
   - **Meeting History Tab**: Dedicated tab showing completed meetings
   - Minutes status indicator: "Đã có" / "Chưa có"
   - Quick access "Xem" button to view minutes directly from history

2. **StudentWidget Enhancements**
   - **Meeting History Tab**: Similar to teacher, shows completed meetings with teacher names
   - **Group Booking Interface**: 
     - Radio buttons: Individual / Group
     - ComboBox auto-loads admin groups
     - Smart validation with detailed error messages
   - **Slot Display**: Now shows max_group_size capacity column
   - **Redesigned Groups Tab**: 
     - Sub-tab 1: "Nhóm của tôi" (My Groups)
     - Sub-tab 2: "Nhóm khác" (Available Groups) with join functionality
     - Sub-tab 3: "Quản lý yêu cầu" (Manage Requests for admins)
     - Sub-tab 4: "Tạo nhóm" (Create Group)

### 📝 Protocol Additions

#### New Commands

- `VIEW_MEETING_HISTORY`: Retrieves completed meetings (status='DONE')
  - Returns: `history` field with meeting details + minutes status
  
- `VIEW_MINUTES`: Fetches meeting minutes content
  - Returns: `minute_id`, `content`, `created_at`, `updated_at`
  - Returns: `minute_id=0` if no minutes exist

#### Modified Commands

- `EDIT_SLOT`: Now validates booking status before allowing modifications
- `BOOK_MEETING_GROUP`: Now validates group member count vs slot capacity
- `REQUEST_JOIN_GROUP`: Now validates group existence first

### 🏗️ Architecture Changes

#### Database (No schema changes required)
- Existing schema supports all new features
- Validations enforce proper usage of existing fields

#### Server Components Modified
- `server/src/handlers/slot_handler.c`: Constraints + protection (~80 lines)
- `server/src/handlers/meeting_handler.c`: Capacity validation (~35 lines)
- `server/src/handlers/group_handler.c`: Existence validation (~15 lines)

#### Client Components Modified
- `client-qt/include/studentwidget.h`: New members and methods
- `client-qt/src/studentwidget.cpp`: UI redesign + booking logic (~180 lines)
- `client-qt/include/teacherwidget.h`: History table member
- `client-qt/src/teacherwidget.cpp`: Minutes view + history (~120 lines)

### 🔧 Technical Details

**Response Codes**:
- `403`: Forbidden - Cannot modify booked slot
- `404`: Not Found - Group doesn't exist
- `400`: Bad Request - Group too large for slot (with details)

**UI Patterns**:
- Tabbed interfaces for better organization
- ComboBox with data binding for dynamic group selection
- Status indicators with color coding
- Quick action buttons embedded in tables

### ✅ Testing Recommendations

1. **Slot Constraints**
   - Create INDIVIDUAL slot → Verify max=1 enforced
   - Try to edit INDIVIDUAL with max=10 → Verify forced to 1

2. **Edit Protection**
   - Book a slot, try to delete → Should fail with 403
   - Cancel meeting, try to delete → Should succeed

3. **Group Capacity**
   - Book with 10-member group on max=5 slot → Should show detailed error
   - Book with 3-member group on max=5 slot → Should succeed

4. **UI Navigation**
   - Test all 4 sub-tabs in Groups section
   - Verify "Xem" buttons navigate correctly
   - Test radio button + ComboBox interaction

### 📊 Statistics

- **Lines Added**: ~430 lines total
- **Files Modified**: 7 files
- **Features Completed**: 7/7 (100%)
- **Compilation**: ✅ Clean, no warnings

### 🚀 Deployment Notes

1. **Server**: Recompile with `make clean && make`
2. **Client**: Build with CMake in build directory
3. **Database**: No migration needed
4. **Testing**: Perform integration tests before production

### 📚 Documentation Updates

- Updated `DESIGN.md` with business rules and validations
- Added this `CHANGELOG.md` file
- Created comprehensive `walkthrough.md` in artifacts
- Updated command reference table

---

**Full Commit Message**:
```
feat: Implement missing features and UI improvements

- Add server-side validations (slot constraints, edit protection, group validation)
- Implement TeacherWidget meeting history and minutes viewing
- Implement StudentWidget group booking UI with validation
- Redesign Groups tab with organized sub-tabs
- Add meeting history displays for both user types
- Update documentation with business rules and new commands

Closes #1, #2, #3, #4, #5, #6, #7
```
