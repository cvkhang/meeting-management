Mô tả ứng dụng
Ứng dụng giúp sinh viên đặt lịch hẹn đúng vào các khe thời gian rảnh do giảng viên công bố, đồng thời hỗ trợ giảng viên quản lý lịch, ghi biên bản và theo dõi lịch sử làm việc với từng sinh viên/nhóm.

Quyền của người dùng
- Sinh viên: 
    + Xem lịch hẹn của mình
    + Đặt lịch hẹn
    + Xem lịch sử làm việc
    + Xem biên bản
- Giảng viên:
    + Xem lịch hẹn của sinh viên
    + Xem lịch sử làm việc
    + Xem biên bản
    + Xem lịch hẹn của nhóm
    + Xem lịch sử làm việc của nhóm
    + Xem biên bản của nhóm

Kiến trúc
  + Client-Server trên TCP:
    + Client: hiển thị UI, kết nối TCP đến server
    + Server: nhận-phân tích lệnh, xác thực, thao tác với CSDL, trả kết quả cho Client
  + CSDL (SQLite) 


Định dạng bản tin
Format
Client → Server (Request):    		COMMAND|payload\r\n	
Server → Client (Response):  		STATUS_CODE|payload\r\n
Server → Client (Notification): 	NTF|payload\r\n
COMMAND: loại bản tin, ví dụ: LOGIN, REGISTER
STATUS_CODE: mã trạng thái, ví dụ: 200
PAYLOAD: các giá trị theo dạng key = value phân tách bởi dấu chấm phẩy ;
Delimiters: 	| phân tách header và payload
      ; phân tách key=value
      = phân tách key và value
      # phân tách các item trong list
      , phân tách các field trong 1 item


Payload - Authentication
Đăng ký:
C→S: REGISTER|role=<r>;username=<u>;password=<p>;full_name=<f>\r\n
S→C: 201|user_id=<id>;msg=Registered\r\n 
  Error: 409|msg=Username_exists\r\n
    400|msg=Missing_fields\r\n
Đăng nhập:
C→S: LOGIN|username=<u>;password=<p>\r\n
S→C: 200|user_id=<id>;token=<token>;msg=Login_OK\r\n
  Error: 404|msg=User_not_found_or_Wrong_password\r\n
  
Token Format: token_<user_id>_<role>


Payload – Slot Management
Khai báo khe thời gian:
C→S: DECLARE_SLOT|token=<t>;date=<yyyy-mm- dd>;start_time=<HH:MM>;end_time=<HH:MM>; slot_type=<type>;max_group_size=<n>
S→C: 201|slot_id=<sid>;msg=Slot_created
  Error: 401|msg=Invalid_token\r\n
    400|msg=Missing_fields\r\n
Sửa khe thời gian:
C→S: EDIT_SLOT|token=<t>;slot_id=<sid>;action=UPDATE;date=<yyyy-mm-dd>;start_time=<HH:MM>;end_time=<HH:MM>;slot_type=<type>;max_group_size=<n>
S→C: 200|slot_id=<sid>;msg=Slot_updated 
Xóa khe thời gian:
C→S: EDIT_SLOT|token=<t>;slot_id=<sid>;action=DELETE
S→C: 200|slot_id=<sid>;msg=Slot_deleted 
  
Error: 404|msg=Slot_not_found_or_Not_owner\r\n



Xem khe thời gian:
C→S: VIEW_SLOTS|teacher_id=<tid>;from_date=<yyyy-mm-dd>;to_date=<yyyy-mm-dd>
S→C: 200|slots=<slot_list>
Trong đó: 	slot_list: slot#slot#slot#...
    slot: slot_id,date,start_time,end_time,slot_type,max_group_size,is_booked
Xem danh sách giáo viên:
C→S: VIEW_TEACHERS|token=<token>\r\n
S→C: 200|teachers=<teacher_list>\r\n
Trong đó: 	teacher_list: t1#t2#t3...
    teacher: user_id,full_name,slot_count
    slot_count = số slot còn trống


Payload - Meeting
Xem lịch họp:
C→S: VIEW_MEETINGS|token=<t>;from_date=<yyyy-mm-dd>;to_date=<yyyy-mm-dd>; status=BOOKED/DONE
S→C: 200|meetings=<meeting_list>
Trong đó: 	meeting_list = m1#m2#m3...
    m = meeting_id,date,start_time,end_time,slot_id,partner_id,meeting_type,status
Ví dụ: 200|meetings=1,2025-11-20,09:00,09:30,2,20220001,INDIVIDUAL,DONE
Hẹn lịch cá nhân:
C→S: BOOK_MEETING_INDIV|token=<t>;teacher_id=<tid>;slot_id=<sid>
S→C: 201|slot_id=<sid>;meeting_id=<mid>;msg=Booked
  Error: 400|msg=Missing_fields\r\n
     409|msg=Slot_already_booked\r\n
Hẹn lịch nhóm:
C→S: BOOK_MEETING_GROUP|token=<t>;teacher_id=<tid>;slot_id=<sid>;group_id=<gid>
S→C: 201|slot_id=<sid>;meeting_id=<mid>;msg=Booked
  Error: 403|msg=Not_group_admin\r\n
    409|msg=Slot_already_booked\r\n
Huỷ lịch họp:
C→S: CANCEL_MEETING|token=<t>;meeting_id=<mid>;reason=<reason>
S→C: 200|meeting_id=<mid>;msg=Cancelled
  Error: 404|msg=Meeting_not_found\r\n
    403|msg=Cannot_cancel_past_meeting\r\n
    403|msg=Not_participant\r\n
Xem chi tiết 1 cuộc họp:
C→S: VIEW_MEETING_DETAIL|token=<t>;meeting_id=<mid>
S→C: 200|meeting_id=<mid>;date=<d>;start_time=<st>;end_time=<et>;teacher_id=<tid>;teacher_name=<>; students/group_id=<id>;students/group_name=<>; meeting_type=<type>;status=<status>;has_minutes=1/0
  Error: 404|msg=Meeting_not_found\r\n
    403|msg=Not_participant\r\n
Xem lịch sử cuộc họp:
C→S: VIEW_MEETING_HISTORY|token=<token>;\r\n
S→C: 200|history=<history_list>\r\n
  h1#h2#h3...
  meeting_id,date,start_time,end_time,partner_name,meeting_type,has_minutes
Lưu biên bản cuộc họp:
C→S: SAVE_MINUTES|token=<token>;meeting_id=<mid>;content=<text_encoded>\r\n
S→C: 201|minute_id=<id>;meeting_id=<mid>;msg=Minutes_saved\r\n
  Error: 400|msg=Content_too_long\r\n
    403|msg=Not_meeting_teacher\r\n
    404|msg=Meeting_not_found\r\n
    409|msg=Minutes_already_exists\r\n
Cập nhật biên bản:
C→S: UPDATE_MINUTES|token=<token>;minute_id=<mid>;content=<text_encoded>\r\n
S→C: 200|minute_id=<mid>;msg=Minutes_updated\r\n
  Error: 403|msg=Not_meeting_teacher\r\n
    404|msg=Minute_not_found\r\n
Xem biên bản cuộc họp:
C→S: VIEW_MINUTES|token=<token>;meeting_id=<mid>\r\n
S→C: 200|minute_id=<mid>;content=<text_encoded>;created_at=<datetime>;updated_at=<datetime>\r\n	200|minute_id=0;msg=No_minutes\r\n
  Error: 404|msg=Meeting_not_found\r\n
    403|msg=Not_participant\r\n




Payload - Group
Tạo nhóm:
C→S: CREATE_GROUP|token=<t>;group_name=<g>
S→C: 201|group_id=<gid> ||  401|msg=Invalid_token
Xem nhóm:
C→S: VIEW_GROUPS|token=<t>
S→C: 200|groups=<group_list>
Trong đó: 	group_list: group#group#group#...
    group: group_id,group_name,member_count,is_member,role
    is_member: 1 = đã vào nhóm, 0 = chưa vào
    role: 1 = admin, 0 = member (chỉ có ý nghĩa khi is_member = 1)
Ví dụ: 200|groups=1,Project 1,4,1,1#2,Đồ án,5,0,0
Xem chi tiết 1 nhóm:
C→S: VIEW_GROUP_DETAIL|token=<t>;group_id=<gid>
S→C: 200|group_id=<gid>;group_name=<name>;members=<member_list>
Trong đó: 	member_list = m1#m2#m3...
    m = student_id,full_name,role
    role 1 = admin, 0 = member
Ví dụ: 200|group_id=1;group_name=Đồ án;members=20220001,Nguyen Van A,1#20220002,Tran Thi B,0#20230003,Pham Van C,0
Yêu cầu vào nhóm:
C→S: REQUEST_JOIN_GROUP|token=<t>;group_id=<gid>;note=<note>
S→C: 202|request_id=<rid>;msg=Request_pending 
  Error: 409|msg=Already_member\r\n
    409|msg=Request_already_exists\r\n
Xem yêu cầu vào nhóm (chỉ lấy PENDING):
C→S: VIEW_JOIN_REQUESTS|token=<t>;group_id=<gid>
S→C: 200|group_id=<gid>;requests=<req_list> 
Trong đó: 	req_list = r1#r2#...
    r = request_id,user_id,full_name,note,status
Ví dụ: 200|group_id=1;requests=10,5,Nguyen Van A,Hello,PENDING
Duyệt\từ chối yêu cầu:
C→S: APPROVE_JOIN_REQUEST|token=<t>;request_id=<rid>
   REJECT_JOIN_REQUEST|token=<t>;request_id=<rid>
S→C: 200|request_id=<rid>;group_id=<gid>;msg=Approved


Payload - Notification
Được duyệt vào nhóm:
S→C: NTF|type=GROUP_APPROVED;group_id=<gid>;group_name=<name>\r\n
  Error: 409|msg=Already_member\r\n
    409|msg=Request_already_exists\r\n
Bị từ chối vào nhóm:
S→C: NTF|type=GROUP_REJECTED;group_id=<gid>;group_name=<name>;reason=<reason>\r\n
Có yêu cầu vào nhóm mới:
S→C: NTF|type=NEW_JOIN_REQUEST;request_id=<>;group_id=<gid>;user_id=<uid>;user_name=<name>; note=<note>\r\n
Có người đặt meeting:
S→C: NTF|type=MEETING_BOOKED;meeting_id=<mid>;slot_id=<sid>; student_name=<name>;meeting_type=<type>\r\n
 Meeting bị hủy:
S→C: NTF|type=MEETING_CANCELLED;meeting_id=<mid>;cancelled_by=<name>;reason=<reason>\r\n
Slot bị cập nhật:
S→C: NTF|type=SLOT_UPDATED;slot_id=<sid>;date=<date>;start_time=<st>;end_time=<et>\r\n
Slot bị xóa:
S→C: NTF|type=SLOT_DELETED;slot_id=<sid>;msg=<message>\r\n
Nhắc nhở meeting:
S→C: NTF|type=MEETING_REMINDER;meeting_id=<mid>;date=<date>; 					time=<time>;with=<partner_name>\r\n

