#pragma once
#include "pch.h"
#include "CorePch.h"
#include "Overlap.h"
#include "Obj.h"

class CLIENT {
public:
    char name[MAX_NAME_SIZE]; //플레이어 id
    int      _id; // array의 저장된 인덱스 <- 서버 관리용
    short  x, y; // 위치
    int _max_hp; // 최대 체력
    int _hp; // 체력
    int _level; // 레벨
    int _max_exp;
    int _exp;
    int _atk; // 공격력
    int _def;
    int _job_no;
    int _gold;
    int _at_range = 1; // 기본 공격 범위
    int _sk_range = 2; // 스킬 범위

    unordered_set   <int>  viewlist; // 시야 안 오브젝트
    mutex vl;

    mutex lua_lock;
    lua_State* L;

    mutex state_lock;
    CL_STATE _state;
    atomic_bool   _is_active;

    COMBAT _combat;

    atomic_int    _count;
    int      _type;   // 1.Player 2.고블린  3.오거 4.드래곤(Boss)   

    Overlap _recv_over;
    SOCKET  _socket;
    int      _prev_size;
    int      last_move_time;
public:
    CLIENT() : _state(ST_FREE),_combat(COMBAT_END) , _prev_size(0)
    {
        x = 0;
        y = 0;
    }



    ~CLIENT()
    {
        closesocket(_socket);
    }

    void do_recv()
    {
        DWORD recv_flag = 0;
        ZeroMemory(&_recv_over._wsa_over, sizeof(_recv_over._wsa_over));
        _recv_over._wsa_buf.buf = reinterpret_cast<char*>(_recv_over._net_buf + _prev_size);
        _recv_over._wsa_buf.len = sizeof(_recv_over._net_buf) - _prev_size;
        int ret = WSARecv(_socket, &_recv_over._wsa_buf, 1, 0, &recv_flag, &_recv_over._wsa_over, NULL);
        if (SOCKET_ERROR == ret) {
            int error_num = WSAGetLastError();
            if (ERROR_IO_PENDING != error_num)
                error_display(error_num);
        }
    }

    void do_send(int num_bytes, void* mess)
    {
        Overlap* ex_over = new Overlap(OP_SEND, num_bytes, mess);
        int ret = WSASend(_socket, &ex_over->_wsa_buf, 1, 0, 0, &ex_over->_wsa_over, NULL);
        if (SOCKET_ERROR == ret) {
            int error_num = WSAGetLastError();
            if (ERROR_IO_PENDING != error_num)
                error_display(error_num);
        }
    }

    void Item_Ability(int _iAtt, int _iHp);
    void Equip_Item(CObj* _pItem);
    void UnEquip_Item(int _iIdx);


    CObj* m_pItem[TYPE_END];
};

