5/9/2026 -- Setup File/Folder

+ Data  ->  Save Entity(Doctor, Patients, Appointment)
        -   Doctor(D_ID, D_LastName, D_MiddleName, D_FirstName, D_PhoneNumbers, D_Gender)
        -   Specialization(D_ID, SP_Description)
        -   Patients(P_ID, P_LastName, P_MiddleName, P_FirstName, P_PHoneNumbers, P_Gender, P_Severity, P_State, P_Date)
        -   Symptom(P_ID, S_Description)
        -   Appointment(A_ID, P_ID, D_ID, A_Date, A_State)
        -   ManagerApp(MA_ID, MA_LastName, MA_MiddleName, MA_FirstName, MA_Role, MA_Gender, MA_PhoneNumbers, Ma_State)
        -   Permission(MA_ID, PM_Description)
+ Giao diện gồm:
        - Account cho loại người dùng (Administrator, Receptionist)
        - Administrator có quyền hạn:   + Cấp quyền
                                        + Kiểm tra toàn bộ thông tin từ khách hàng đến bác sĩ.
                                        + Chỉnh sửa thông tin tài khoản (trừ bản thân và những tài khoản cùng cấp)
                                        + Cấu hình hệ thống
        - Receptionist có quyền hạn:    + Kiểm tra thông tin khách hàng (không thể chỉnh sửa khi chưa được Administrator cấp quyền)
                                        + Xếp lịch
        
        - Các options:
                + Quản lí accounts.     (role: 3)
                + Đặt lịch.             (role: 1)
                + Kiểm tra thông tin.   (role: 2x)
                + Thống kê.             (role: 1)
                + Config.               (role: 2x)
        
        Role: 1  - Receptionist
              2x - Receptionist + quyền được cấp
              3  - Adminstrator
        
        Quản lí account gồm:
                + Thông tin tài khoản tồn tại.
                + |Ten|Role (Receptionist _ Adminstrator)|State|Phone|Action|
                + Chỉnh sửa thông tin account
                + Role 2x ~ 1 vì 2x là 1 nhưng được cấp quyền phụ
                
        Đặt lịch gồm:
                + Một button Create calender.
                + Lịch (Tháng, tuần, ngày).
                        - Năm show tháng, (Tháng, Tuần) show ngày.
                        - Có tương tác với lịch (chọn năm, tháng, tuần, ngày, xem lịch bác sĩ)
                + Sửa lịch (Có thể).
        
        Kiểm tra thông tin (Patients & Doctors):
                + Option: Patients & Doctor
                + Findbar: Tên
                + Sort:
                - Patients:     + sort theo date khám gần nhất và chưa được khám.
                                + lọc theo đã được khám.
                - Doctors:      + sort theo lịch khám gần nhất (dựa vào patient để tra)
        
        Thống kê:
                + Show số lượng đã khám theo thời gian (xxxx->yyyy) hoặc năm, tháng, tuần, ngày và show số lượng bệnh nhân mới tiếp nhận, bác sĩ còn hành nghề, tỉ lệ khám đúng hẹn.
                + Show sơ đồ thống kê theo thời gian (xxxx->yyyy) hoặc theo năm, tháng, tuần, ngày. Gồm số lượng đá khám + số lượng bệnh nhân mới tiếp nhận.
                + Show tỉ lệ theo sơ đồ tròn.

        Config: 
                + ... idk
        
        LoginUI:
                + Có login, register, forgotten password.

9/9/2026 --Phân công
+ Remove:       - Config
+ Add:          - Thêm giới tính cho Doctor | ManagerApp
                - Thêm tạo thẻ khách hàng
        - Giao diện:
                - Quản lí tài khoản:
                        | Có Search, SlideBar
                - Đặt lịch:
                        | Có SlideBar, Search Date,
        

