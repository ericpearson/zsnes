;Copyright (C) 1997-2008 ZSNES Team ( zsKnight, _Demo_, pagefault, Nach )
;
;http://www.zsnes.com
;http://sourceforge.net/projects/zsnes
;https://zsnes.bountysource.com
;
;This program is free software; you can redistribute it and/or
;modify it under the terms of the GNU General Public License
;version 2 as published by the Free Software Foundation.
;
;This program is distributed in the hope that it will be useful,
;but WITHOUT ANY WARRANTY; without even the implied warranty of
;MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;GNU General Public License for more details.
;
;You should have received a copy of the GNU General Public License
;along with this program; if not, write to the Free Software
;Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.



%include "macros.mac"

EXTSYM JoyRead
EXTSYM soundon,DSPDisable,Start60HZ
EXTSYM GetMouseX,GetMouseY,GetMouseMoveX
EXTSYM GetMouseMoveY,GetMouseButton,SetMouseMinX,SetMouseMaxX,SetMouseMinY
EXTSYM SetMouseMaxY,SetMouseX,SetMouseY,T36HZEnabled,MouseButton,Start36HZ
EXTSYM BufferSizeW,BufferSizeB,ProcessSoundBuffer,CheckTimers
EXTSYM FrameSemaphore
EXTSYM pl1upk,pl1downk,pl1leftk,pl1rightk,pl1startk,pl1selk
EXTSYM pl1Ak,pl1Bk,pl1Xk,pl1Yk,pl1Lk,pl1Rk
EXTSYM pl2upk,pl2downk,pl2leftk,pl2rightk,pl2startk,pl2selk
EXTSYM pl2Ak,pl2Bk,pl2Xk,pl2Yk,pl2Lk,pl2Rk
EXTSYM pl3upk,pl3downk,pl3leftk,pl3rightk,pl3startk,pl3selk
EXTSYM pl3Ak,pl3Bk,pl3Xk,pl3Yk,pl3Lk,pl3Rk
EXTSYM pl4upk,pl4downk,pl4leftk,pl4rightk,pl4startk,pl4selk
EXTSYM pl4Ak,pl4Bk,pl4Xk,pl4Yk,pl4Lk,pl4Rk
EXTSYM pl5upk,pl5downk,pl5leftk,pl5rightk,pl5startk,pl5selk
EXTSYM pl5Ak,pl5Bk,pl5Xk,pl5Yk,pl5Lk,pl5Rk

; NOTE: For timing, Game60hzcall should be called at 50hz or 60hz (depending
;   on romispal) after a call to InitPreGame and before DeInitPostGame are
;   made.  GUI36hzcall should be called at 36hz after a call GUIInit and
;   before GUIDeInit.

SECTION .data
NEWSYM CurKeyPos, dd 0
NEWSYM CurKeyReadPos, dd 0
NEWSYM KeyBuffer, times 16 dd 0
SECTION .text

; ****************************
; Video Stuff
; ****************************

; ** copy video mode functions **
SECTION .data
NEWSYM converta, dd 0

; ** Video Mode Variables **
SECTION .data

; Total Number of Video Modes
NEWSYM NumVideoModes, dd 60

; GUI Video Mode Names - Make sure that all names are of the same length
; and end with a NULL terminator
NEWSYM GUIVideoModeNames
db '256x224       R W',0  ;0
db '256x224       R F',0  ;1
db '512x448       R W',0  ;2
db '512x448      DR W',0  ;3
db '640x480       S W',0  ;4
db '640x480      DS W',0  ;5
db '640x480      DR F',0  ;6
db '640x480      DS F',0  ;7
db '640x480       S F',0  ;8
db '768x672       R W',0  ;9
db '768x672      DR W',0  ;10
db '800x600       S W',0  ;11
db '800x600      DS W',0  ;12
db '800x600       S F',0  ;13
db '800x600      DR F',0  ;14
db '800x600      DS F',0  ;15
db '1024x768      S W',0  ;16
db '1024x768     DS W',0  ;17
db '1024x768      S F',0  ;18
db '1024x768     DR F',0  ;19
db '1024x768     DS F',0  ;20
db '1024x896      R W',0  ;21
db '1024x896     DR W',0  ;22
db '1280x960      S W',0  ;23
db '1280x960     DS W',0  ;24
db '1280x960      S F',0  ;25
db '1280x960     DR F',0  ;26
db '1280x960     DS F',0  ;27
db '1280x1024     S W',0  ;28
db '1280x1024    DS W',0  ;29
db '1280x1024     S F',0  ;30
db '1280x1024    DR F',0  ;31
db '1280x1024    DS F',0  ;32
db '1600x1200     S W',0  ;33
db '1600x1200    DS W',0  ;34
db '1600x1200    DR F',0  ;35
db '1600x1200    DS F',0  ;36
db '1600x1200     S F',0  ;37
db 'CUSTOM       D  W',0  ;38
db 'CUSTOM       DS F',0  ;39
db 'CUSTOM          W',0  ;40
db 'CUSTOM        S F',0  ;41
db 'CUSTOM       DR F',0  ;42
db '512x448     ODR W',0  ;43
db '640x480     ODS F',0  ;44
db '640x480     ODS W',0  ;45
db '768x672     ODR W',0  ;46
db '800x600     ODS F',0  ;47
db '800x600     ODS W',0  ;48
db '1024x768    ODS F',0  ;49
db '1024x768    ODS W',0  ;50
db '1024x896    ODR W',0  ;51
db '1280x960    ODS F',0  ;52
db '1280x960    ODS W',0  ;53
db '1280x1024   ODS F',0  ;54
db '1280x1024   ODS W',0  ;55
db '1600x1200   ODS F',0  ;56
db '1600x1200   ODS W',0  ;57
db 'CUSTOM      OD  F',0  ;58
db 'VARIABLE    OD  W',0  ;59


; Video Mode Feature Availability (1 = Available, 0 = Not Available)
; Left side starts with Video Mode 0
;                    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9
NEWSYM GUIWFVID,  db 0,1,0,0,0,0,1,1,1,0,0,0,0,1,1,1,0,0,1,1,1,0,0,0,0,1,1,1,0,0,1,1,1,0,0,1,1,1,0,1,0,1,1,0,1,0,0,1,0,1,0,0,1,0,1,0,1,0,1,0 ; Fullscreen
NEWSYM GUIDSIZE,  db 0,0,0,1,0,1,1,1,0,0,1,0,1,0,1,1,0,1,0,1,1,0,1,0,1,0,1,1,0,1,0,1,1,0,1,1,1,0,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 ; D Modes
NEWSYM GUISMODE,  db 0,0,0,0,1,0,0,0,1,0,0,1,0,1,0,0,1,0,1,0,0,0,0,1,0,1,0,0,1,0,1,0,0,1,0,0,0,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 ; Win Stretched Modes
NEWSYM GUIDSMODE, db 0,0,0,0,0,1,0,1,0,0,0,0,1,0,0,1,0,1,0,0,1,0,0,0,1,0,0,1,0,1,0,0,1,0,1,0,1,0,1,1,0,0,0,0,1,1,0,1,1,1,1,0,1,1,1,1,1,1,1,1 ; Win D-Stretched Modes
NEWSYM GUIKEEP43, db 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,1,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1 ; Keep 4:3 Ratio
NEWSYM GUIM7VID,  db 0,0,0,1,0,1,1,1,0,0,1,0,1,0,1,1,0,1,0,1,1,0,1,0,1,0,1,1,0,1,0,1,1,0,1,1,1,0,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 ; Hires Mode 7
NEWSYM GUIHQ2X,   db 0,0,0,1,0,1,1,1,0,0,1,0,1,0,1,1,0,1,0,1,1,0,1,0,1,0,1,1,0,1,0,1,1,0,1,1,1,0,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 ; Hq2x Filter
NEWSYM GUIHQ3X,   db 0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,1,0,1,0,1,1,0,1,0,1,0,1,1,0,1,0,1,1,0,1,1,1,0,1,1,0,0,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1 ; Hq3x Filter
NEWSYM GUIHQ4X,   db 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,1,0,1,0,1,1,0,1,1,1,0,1,1,0,0,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1 ; Hq4x Filter
NEWSYM GUINTVID,  db 0,0,0,0,0,1,1,1,0,0,1,0,1,0,1,1,0,1,0,1,1,0,1,0,1,0,1,1,0,1,0,1,1,0,1,1,1,0,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 ; NTSC Filter
NEWSYM GUIBIFIL,  db 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 ; Bilinear Filter

SECTION .text

; ****************************
; Input Device Stuff
; ****************************

; Variables related to Input Device Routines:
;   pl1selk,pl1startk,pl1upk,pl1downk,pl1leftk,pl1rightk,pl1Xk,
;   pl1Ak,pl1Lk,pl1Yk,pl1Bk,pl1Rk
;     (Change 1 to 2,3,4 for other players)
;     Each of these variables contains the corresponding key pressed value
;       for the key data
;   pressed[]
;     - This is an array of pressed/released data (bytes) where the
;       corresponding key pressed value is used as the index.  The value
;       for each entry is 0 for released and 1 for pressed.  Also, when
;       writing keyboard data to this array, be sure to first check if
;       the value of the array entry is 2 or not.  If it is 2, do not write 1
;       to that array entry. (however, you can write 0 to it)
;   As an example, to access Player 1 L button press data, it is
;     done like : pressed[pl1Lk]
;   The 3 character key description of that array entry is accessed by the
;     GUI through ScanCodeListing[pl1Lk*3]

; Note: When storing the input device configuration of a dynamic input
;   device system (ie. Win9x) rather than a static system (ie. Dos), it
;   is best to store in the name of the device and relative button
;   assignments in the configuration file, then convert it to ZSNES'
;   numerical corresponding key format after reading from it. And then
;   convert it back when writing to it back.

SECTION .data

; Total Number of Input Devices
NEWSYM NumInputDevices, dd 2

; Input Device Names
NEWSYM GUIInputNames
db 'NONE            ',0
db 'KEYBOARD/GAMEPAD',0
db '                ',0
db '                ',0
db '                ',0
db '                ',0
db '                ',0
db '                ',0
db '                ',0
db '                ',0
db '                ',0
db '                ',0
db '                ',0
db '                ',0
db '                ',0
db '                ',0

; GUI Description codes for each corresponding key pressed value
NEWSYM ScanCodeListing
        db '---','ESC',' 1 ',' 2 ',' 3 ',' 4 ',' 5 ',' 6 '  ; 00h
        db ' 7 ',' 8 ',' 9 ',' 0 ',' - ',' = ','BKS','TAB'
        db ' Q ',' W ',' E ',' R ',' T ',' Y ',' U ',' I '  ; 10h
        db ' O ',' P ',' [ ',' ] ','RET','LCT',' A ',' S '
        db ' D ',' F ',' G ',' H ',' J ',' K ',' L ',' : '  ; 20h
        db ' " ',' ~ ','LSH',' \ ',' Z ',' X ',' C ',' V '
        db ' B ',' N ',' M ',' , ',' . ',' / ','RSH',' * '  ; 30h
        db 'LAL','SPC','CAP','F1 ','F2 ','F3 ','F4 ','F5 '
        db 'F6 ','F7 ','F8 ','F9 ','F10','NUM','SCR','KP7'  ; 40h
        db 'KP8','KP9','KP-','KP4','KP5','KP6','KP+','KP1'
        db 'KP2','KP3','KP0','KP.','   ','   ','OEM','F11'  ; 50h
        db 'F12','59H','5AH','5BH','5CH','5DH','5EH','5FH'
        db '60H','61H','62H','63H','64H','65H','66H','67H'  ; 60h
        db '68H','69H','6AH','6BH','6CH','6DH','6EH','6FH'
        db '70H','71H','72H','73H','74H','75H','76H','77H'  ; 70h
        db '78H','79H','7AH','7BH','7CH','7DH','7EH','7FH'
        ; Keyboard continued (Direct Input)
        db '80H','81H','82H','83H','84H','85H','86H','87H'  ; 80h
        db '88H','89H','8AH','8BH','8CH','8DH','8EH','8FH'
        db '90H','91H','92H','93H','94H','95H','96H','97H'  ; 90h
        db '98H','99H','9AH','9BH','9CH','9DH','9EH','9FH'
        db 'A0H','A1H','A2H','A3H','A4H','A5H','A6H','A7H'  ; A0h
        db 'A8H','A9H','AAH','ABH','ACH','ADH','AEH','AFH'
        db 'B0H','B1H','B2H','B3H','B4H','B5H','B6H','B7H'  ; B0h
        db 'B8H','B9H','BAH','BBH','BCH','BDH','BEH','BFH'
        db 'C0H','C1H','C2H','C3H','C4H','C5H','C6H','C7H'  ; C0h
        db 'C8H','C9H','CAH','CBH','CCH','CDH','CEH','CFH'
        db 'D0H','D1H','D2H','D3H','D4H','D5H','D6H','D7H'  ; D0h
        db 'D8H','D9H','DAH','DBH','DCH','DDH','DEH','DFH'
        db 'E0H','E1H','E2H','E3H','E4H','E5H','E6H','E7H'  ; E0h
        db 'E8H','E9H','EAH','EBH','ECH','EDH','EEH','EFH'
        db 'F0H','F1H','F2H','F3H','F4H','F5H','F6H','F7H'  ; F0h
        db 'F8H','F9H','FAH','FBH','FCH','FDH','FEH','FFH'
        ; Joystick Stuff (Direct Input)
        db 'J00','J01','J02','J03','J04','J05','J06','J07'
        db 'J08','J09','J0A','J0B','J0C','J0D','J0E','J0F'
        db 'J10','J11','J12','J13','J14','J15','J16','J17'
        db 'J18','J19','J1A','J1B','J1C','J1D','J1E','J1F'
        db 'J20','J21','J22','J23','J24','J25','J26','J27'
        db 'J28','J29','J2A','J2B','J2C','J2D','J2E','J2F'
        db 'J30','J31','J32','J33','J34','J35','J36','J37'
        db 'J38','J39','J3A','J3B','J3C','J3D','J3E','J3F'
        db 'J40','J41','J42','J43','J44','J45','J46','J47'
        db 'J48','J49','J4A','J4B','J4C','J4D','J4E','J4F'
        db 'J50','J51','J52','J53','J54','J55','J56','J57'
        db 'J58','J59','J5A','J5B','J5C','J5D','J5E','J5F'
        db 'J60','J61','J62','J63','J64','J65','J66','J67'
        db 'J68','J69','J6A','J6B','J6C','J6D','J6E','J6F'
        db 'J70','J71','J72','J73','J74','J75','J76','J77'
        db 'J78','J79','J7A','J7B','J7C','J7D','J7E','J7F'
        ; Extra Stuff (180h) (Parallel Port)
        db 'PPB','PPY','PSL','PST','PUP','PDN','PLT','PRT'
        db 'PPA','PPX','PPL','PPR','   ','   ','   ','   '
        db 'P2B','P2Y','P2S','P2T','P2U','P2D','P2L','P2R'
        db 'P2A','P2X','P2L','P2R','   ','   ','   ','   '
        db 'PPB','PPY','PSL','PST','PUP','PDN','PLT','PRT'
        db 'PPA','PPX','PPL','PPR','   ','   ','   ','   '
        db 'P2B','P2Y','P2S','P2T','P2U','P2D','P2L','P2R'
        db 'P2A','P2X','P2L','P2R','   ','   ','   ','   '

SECTION .text

; ****************************
; Mouse Stuff
; ****************************

SECTION .data
NEWSYM WMouseX, dd 0
NEWSYM WMouseY, dd 0
NEWSYM WMouseMoveX, dd 0
NEWSYM WMouseMoveY, dd 0
NEWSYM WMouseButton, dd 0

SECTION .text

NEWSYM Get_MouseData         ; Returns both pressed and coordinates
    ; bx : bit 0 = left button, bit 1 = right button
    ; cx = Mouse X Position, dx = Mouse Y Position
    push eax
    ccall GetMouseX
    mov [WMouseX],eax
    ccall GetMouseY
    mov [WMouseY],eax
    ccall GetMouseButton
    mov [WMouseButton],eax
    pop eax
    mov cx,[WMouseX]
    mov dx,[WMouseY]
    mov bx,[WMouseButton]
    ret

NEWSYM Set_MouseXMax    ; Sets the X boundaries (ecx = left, edx = right)
    ccallv SetMouseMinX, ecx
    ccallv SetMouseMaxX, edx
    ret

NEWSYM Set_MouseYMax    ; Sets the Y boundaries (ecx = left, edx = right)
    ccallv SetMouseMinY, ecx
    ccallv SetMouseMaxY, edx
    ret

NEWSYM Set_MousePosition        ; Sets Mouse Position (x:cx,y:dx)
    ccallv SetMouseX, ecx
    ccallv SetMouseY, edx
    ret

NEWSYM Get_MousePositionDisplacement
    ; returns x,y displacement in pixel in cx,dx
    push eax
    ccall GetMouseMoveX
    mov [WMouseMoveX],eax
    ccall GetMouseMoveY
    mov [WMouseMoveY],eax
    pop eax
    mov cx,[WMouseMoveX]
    mov dx,[WMouseMoveY]
    ret

NEWSYM MouseWindow
    or byte[MouseButton],2
    mov byte[T36HZEnabled],1
    ccallv GetMouseButton
    and byte[MouseButton],0FDh
    ret

; ****************************
; Sound Stuff
; ****************************

NEWSYM StopSound
    ccallv Start36HZ
    ccallv JoyRead
    ret

NEWSYM StartSound
    ccallv Start60HZ
    ccallv JoyRead
    ret


NEWSYM SoundProcess     ; This function is called ~60 times/s at full speed
    cmp byte[soundon],0
    je .nosound
    cmp byte[DSPDisable],1
    je .nosound
    mov eax,256         ; Size
    mov [BufferSizeB],eax
    add eax,eax
    mov [BufferSizeW],eax
    pushad
    call ProcessSoundBuffer
    popad
    ; DSPBuffer should contain the processed buffer in the specified size
    ; You will have to convert/clip it to 16-bit for actual sound process
.nosound
    ret

NEWSYM Check60hz
    ; Call the timer update function here
    ccallv CheckTimers
    ccallv FrameSemaphore
    ret

%macro SetDefaultKey2 13
  mov dword[%1upk],%4    ; Up
  mov dword[%1downk],%5  ; Down
  mov dword[%1leftk],%6  ; Left
  mov dword[%1rightk],%7 ; Right
  mov dword[%1startk],%3 ; Start
  mov dword[%1selk],%2   ; Select
  mov dword[%1Ak],%9     ; A
  mov dword[%1Bk],%12    ; B
  mov dword[%1Xk],%8     ; X
  mov dword[%1Yk],%11    ; Y
  mov dword[%1Lk],%10    ; L
  mov dword[%1Rk],%13    ; R
%endmacro

%macro SetDefaultKey 12
  cmp bh,0
  jne %%nopl1
  SetDefaultKey2 pl1,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12
%%nopl1
  cmp bh,1
  jne %%nopl2
  SetDefaultKey2 pl2,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12
%%nopl2
  cmp bh,2
  jne %%nopl3
  SetDefaultKey2 pl3,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12
%%nopl3
  cmp bh,3
  jne %%nopl4
  SetDefaultKey2 pl4,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12
%%nopl4
  cmp bh,4
  jne %%nopl5
  SetDefaultKey2 pl5,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12
%%nopl5
%endmacro

NEWSYM SetInputDevice
    ; bl = device #, bh = player # (0-4)
    ; Sets keys according to input device selected
    cmp bl,0
    jne near .nozero
    SetDefaultKey 0,0,0,0,0,0,0,0,0,0,0,0
    ret
.nozero
    cmp bh,1
    je near .input2
    SetDefaultKey 54,28,200,208,203,205,31,45,32,30,44,46
    ret
.input2
    SetDefaultKey 56,29,36,50,49,51,210,199,201,211,207,209
    ret
