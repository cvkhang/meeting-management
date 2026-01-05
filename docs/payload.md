2.6.4 Danh sách Commands
	2.6.4.1 Authentication
Đăng ký
C→S: 		REGISTER|role=<r>;username=<u>;password=<p>;full_name=<f>\r\n
S→C: 		201|user_id=<id>;msg=Registered\r\n
     	Error: 409|msg=Username_exists\r\n
            	400|msg=Missing_fields\r\n
Đăng nhập
C→S: 		LOGIN|username=<u>;password=<p>\r\n
S→C: 		200|user_id=<id>;role=<r>;token=<token>;msg=Login_OK\r\n
  	Error: 404|msg=User_not_found_or_Wrong_password\r\n

	2.6.4.2 Slot Management
Tạo slot mới
C→S:DECLARE_SLOT|token=<t>;date=<d>;start_time=<st>;end_time=<et>;slot_type=<type>;max_group_size=<max>\r\n
S→C: 201|slot_id=<id>;msg=Slot_created\r\n
Error: 409|msg=Time_conflict;conflicting_slot=<id>\r\n
           400|msg=Missing_fields\r\n
           401|msg=Invalid_token\r\n

Xem danh sách giáo viên
C→S: VIEW_TEACHERS\r\n
S→C: 200|teachers=<t1>#<t2>#...\r\n
Teacher format:user_id,full_name,available_slot_count

Xem slots
C→S: VIEW_SLOTS|teacher_id=<tid>;from_date=<date>;to_date=<date>\r\n
S→C: 200|slots=<slot1>#<slot2>#...\r\n
Slot format:slot_id,date,start_time,end_time,slot_type,max_group_size,status
Status: AVAILABLE | BOOKED

Sửa slot
C→S:EDIT_SLOT|token=<t>;slot_id=<sid>;action=UPDATE;date=<d>;start_time=<st>;end_time=<et>;slot_type=<type>;max_group_size=<max>\r\n
S→C: 200|slot_id=<sid>;msg=Slot_updated\r\n
 Error: 404|msg=Slot_not_found_or_Not_owner\r\n
            403|msg=Cannot_modify_booked_slot\r\n
            409|msg=Time_conflict;conflicting_slot=<id>\r\n

Xóa slot
C→S: EDIT_SLOT|token=<t>;slot_id=<sid>;action=DELETE\r\n
S→C: 200|slot_id=<sid>;msg=Slot_deleted\r\n
 Error: 404|msg=Slot_not_found_or_Not_owner\r\n
            403|msg=Cannot_modify_booked_slot\r\n

	2.6.4.3 Meeting Management
Đặt lịch cá nhân
C→S: BOOK_MEETING_INDIV|token=<t>;teacher_id=<tid>;slot_id=<sid>\r\n
S→C: 201|slot_id=<sid>;meeting_id=<mid>;msg=Booked\r\n
 Error: 409|msg=Time_conflict;conflicting_meeting=<mid>\r\n
            409|msg=Slot_already_booked\r\n
            404|msg=Slot_not_found\r\n
            400|msg=Missing_fields\r\n

NTF→Teacher:NTF|type=MEETING_BOOKED;meeting_id=<mid>;slot_id=<sid> \r\n

Đặt lịch theo nhóm
C→S:BOOK_MEETING_GROUP|token=<t>;teacher_id=<tid>;slot_id=<sid>;group_id=<gid>\r\n
S→C:  201|slot_id=<sid>;meeting_id=<mid>;msg=Booked\r\n
 Error: 403|msg=Not_group_admin\r\n
            409|msg=Time_conflict;conflicting_meeting=<mid>\r\n
            409|msg=Slot_already_booked\r\n
            409|msg=Group_size_exceeded;max=<max>;actual=<count>\r\n
            404|msg=Slot_not_found\r\n

NTF→Teacher: NTF|type=MEETING_BOOKED;meeting_id=<mid>;slot_id=<sid>;group_id=<gid>;meeting_type=GROUP\r\n
NTF→All Members: NTF|type=MEETING_BOOKED;meeting_id=<mid>;slot_id=<sid>;group_id=<gid>;meeting_type=GROUP\r\n

Xem danh sách meeting
C→S: VIEW_MEETINGS|token=<t>;from_date=<date>;to_date=<date> \r\n
S→C: 200|meetings=<m1>#<m2>#...\r\n
Có thể không gửi date range
Meeting format: meeting_id, date, start_time, end_time, slot_id, teacher_id, meeting_type, status

Hủy meeting
C→S: CANCEL_MEETING|token=<t>;meeting_id=<mid>;reason=<r>\r\n
S→C: 200|meeting_id=<mid>;msg=Cancelled\r\n
 Error: 404|msg=Meeting_not_found\r\n
            403|msg=Permission_denied\r\n

- Teacher: Có thể cancel tất cả meetings của mình
- Student: Cancel meetings cá nhân của mình
- Group admin: Cancel group meetings (members thường không thể)

NTF → Teacher & Student: NTF|type=MEETING_CANCELLED;meeting_id=<mid>;cancelled_by=<uid>\r\n

Hoàn thành meeting
C→S: COMPLETE_MEETING|token=<teacher_token>;meeting_id=<mid>\r\n
S→C: 200|meeting_id=<mid>;msg=Completed\r\n
 Error: 404|msg=Meeting_not_found\r\n
            403|msg=Not_meeting_teacher\r\n
            400|msg=Invalid_status;current=<status>\r\n

	2.6.4.4 Group Management
Tạo nhóm
C→S: CREATE_GROUP|token=<t>;group_name=<name>\r\n
S→C: 201|group_id=<gid>\r\n
 Error: 401|msg=Invalid_token_or_Missing_fields\r\n

Xem nhóm
C→S: VIEW_GROUPS|token=<t>\r\n
S→C: 200|groups=<g1>#<g2>#...\r\n
Group format: group_id,group_name,member_count,is_member,role
- is_member: 1 nếu user là member, 0 nếu không
- role: 1 = admin, 0 = member (chỉ có ý nghĩa khi is_member=1)


Xem chi tiết nhóm
C→S: VIEW_GROUP_DETAIL|token=<t>;group_id=<gid>\r\n
S→C: 200|group_id=<gid>;group_name=<name>;members=<m1>#<m2>#...\r\n
 Error: 404|msg=Group_not_found\r\n
Member format: user_id,full_name,is_admin

Yêu cầu vào nhóm
C→S: REQUEST_JOIN_GROUP|token=<t>;group_id=<gid>;note=<note>\r\n
S→C: 202|request_id=<rid>;msg=Request_pending\r\n
 Error: 409|msg=Already_member\r\n
            404|msg=Group_not_found\r\n
            409|msg=Request_already_exists\r\n

NTF → Admin: NTF|type=NEW_JOIN_REQUEST; request_id=<rid>;group_id=<gid>;user_id=<uid>;username=<name>; group_name=<name>;note=<note>\r\n

Xem Request (Admin Only)
C→S: VIEW_JOIN_REQUESTS|token=<admin_token>;group_id=<gid>\r\n
S→C: 200|group_id=<gid>;requests=<r1>#<r2>#...\r\n
 Error: 403|msg=Not_admin\r\n

Request format: request_id,user_id,full_name,note,status

Duyệt request
C→S: APPROVE_JOIN_REQUEST|token=<admin_token>;request_id=<rid>\r\n
S→C: 200| request_id=<rid>;group_id=<gid>;msg=Approved\r\n
 Error: 403|msg=Not_admin\r\n

NTF → Requester: NTF|type=GROUP_APPROVED;group_id=<gid>;request_id=<rid>\r\n

Từ chối request
C→S: REJECT_JOIN_REQUEST|token=<admin_token>;request_id=<rid>\r\n
S→C: 200| request_id=<rid>;group_id=<gid>;msg=Rejected\r\n
 Error: 403|msg=Not_admin\r\n

NTF → Requester: NTF|type=GROUP_REJECTED;group_id=<gid>;request_id=<rid>\r\n

	2.6.4.5 Minutes Management
Lưu biên bản
C→S: SAVE_MINUTES|token=<t>;meeting_id=<mid>;content=<text>\r\n
S→C: 201|minute_id=<minid>;meeting_id=<mid>;msg=Minutes_saved;status=DONE\r\n
 Error: 400|msg=Missing_fields\r\n
            403|msg=Permission_denied\r\n

Xem biên bản
C→S: VIEW_MINUTES|token=<t>;meeting_id=<mid>\r\n
S→C: 200|minute_id=<minid>;content=<text>;created_at=<timestamp>; updated_at=<timestamp>\r\n
	200|minute_id=0;msg=No_minutes
 Error: 404|msg=Minutes_not_found\r\n
            403|msg=Permission_denied\r\n

Cập nhật biên bản
C→S: UPDATE_MINUTES|token=<t>;minute_id=<minid>;content=<text>\r\n
S→C: 200|minute_id=<minid>;msg=Minutes_updated\r\n
 Error: 404|msg=Minutes_not_found\r\n
            403|msg=Permission_denied\r\n
 
