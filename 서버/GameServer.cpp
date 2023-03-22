//----------------------------------------------------------------------------------------------------------------------------------------------
//2021/12/08
// 2017182043 최윤태
// 겜서버 텀프
//----------------------------------------------------------------------------------------------------------------------------------------------

#include "pch.h"
#include "CorePch.h"
#include "CLIENT.h"
#include "Overlap.h"
#include "Enum.h"

#include <sqlext.h>  



#define UNICODE  // 우리나라는 Default니깐 굳이 안써도 됨
#define NAME_LEN 20  
#define PHONE_LEN 60


HANDLE g_h_iocp;
SOCKET sever_socket;

concurrency::concurrent_priority_queue <timer_ev> timer_q;
array <CLIENT, MAX_USER + MAX_NPC> clients;

void do_npc_move(int npc_id, int user_id);
void do_npc_attack(int npc_id, int user_id);

void show_err() {
	cout << "error" << endl;
}

//DB 에러 출력
void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
	setlocale(LC_ALL, "korean");
	std::wcout.imbue(std::locale("korean"));
	SQLSMALLINT iRec = 0;
	SQLINTEGER iError;
	WCHAR wszMessage[1000];
	WCHAR wszState[SQL_SQLSTATE_SIZE + 1];
	if (RetCode == SQL_INVALID_HANDLE) {
		fwprintf(stderr, L"Invalid handle! n");
		return;
	}

	while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage, (SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT*)NULL) == SQL_SUCCESS) {
		// Hide data truncated..
		if (wcsncmp(wszState, L"01004", 5)) {
			wprintf(L"[%s] : ", wszState);
			wprintf(L"%s", wszMessage);
			wprintf(L" - %d\n", iError);
		}
	}
}

//플레이어 데이터 받아오기
bool DB_odbc(int id, char* name)  
{
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0;
	SQLRETURN retcode;
	cout << name << endl;
	char temp[BUFSIZE];
	sprintf_s(temp, sizeof(temp), "EXEC select_as_id %s", name);
	wchar_t* exec;
	int str_size = MultiByteToWideChar(CP_ACP, 0, temp, -1, NULL, NULL);
	exec = new WCHAR[str_size];
	MultiByteToWideChar(CP_ACP, 0, temp, sizeof(temp) + 1, exec, str_size);

	SQLINTEGER p_x;
	SQLINTEGER p_y;
	SQLINTEGER p_m_hp;
	SQLINTEGER p_hp;
	SQLINTEGER p_level;
	SQLINTEGER p_m_exp;
	SQLINTEGER p_exp;
	SQLINTEGER p_atk;
	SQLINTEGER p_def;
	SQLINTEGER p_job;
	SQLINTEGER p_gold;

	SQLWCHAR p_id[NAME_LEN];
	SQLLEN cbP_ID = 0, cbP_X = 0, cbP_Y = 0, cbP_MAX_HP = 0, cbP_HP = 0, 
		cbP_MAX_EXP = 0, cbP_EXP = 0, cbP_LEVEL = 0, cbP_ATK = 0, cbP_DEF = 0, cbP_JOB_NUM = 0, cbP_GOLD = 0;
	
	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2021_Class_ODBC", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					cout << "ODBC Connection Success" << endl;
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
					retcode = SQLExecDirect(hstmt, (SQLWCHAR*)exec, SQL_NTS);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

						// Bind columns 1, 2, and 3  
						retcode = SQLBindCol(hstmt, 1, SQL_C_WCHAR, p_id, NAME_LEN, &cbP_ID);
						retcode = SQLBindCol(hstmt, 2, SQL_C_LONG, &p_x, 100, &cbP_X);
						retcode = SQLBindCol(hstmt, 3, SQL_C_LONG, &p_y, 100, &cbP_Y);
						retcode = SQLBindCol(hstmt, 4, SQL_C_LONG, &p_m_hp, 100, &cbP_MAX_HP);
						retcode = SQLBindCol(hstmt, 5, SQL_C_LONG, &p_hp, 100, &cbP_HP);
						retcode = SQLBindCol(hstmt, 6, SQL_C_LONG, &p_level, 100, &cbP_LEVEL);
						retcode = SQLBindCol(hstmt, 7, SQL_C_LONG, &p_m_exp, 100, &cbP_MAX_EXP);
						retcode = SQLBindCol(hstmt, 8, SQL_C_LONG, &p_exp, 100, &cbP_EXP);
						retcode = SQLBindCol(hstmt, 9, SQL_C_LONG, &p_atk, 100, &cbP_ATK);
						retcode = SQLBindCol(hstmt, 10, SQL_C_LONG, &p_def, 100, &cbP_DEF);
						retcode = SQLBindCol(hstmt, 11, SQL_C_LONG, &p_job, 100, &cbP_JOB_NUM);
						retcode = SQLBindCol(hstmt, 12, SQL_C_LONG, &p_gold, 100, &cbP_GOLD);

						// Fetch and print each row of data. On an error, display a message and exit.  
						for (int i = 0; ; i++) {
							retcode = SQLFetch(hstmt);
							if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
								show_err();
							if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
							{
								//replace wprintf with printf
								//%S with %ls
								//warning C4477: 'wprintf' : format string '%S' requires an argument of type 'char *'
								//but variadic argument 2 has type 'SQLWCHAR *'
								//wprintf(L"%d: %S %S %S\n", i + 1, sCustID, szName, szPhone);
								//strcpy_s(clients[id].name, (char*)p_id);
								
								if (p_id != 0) {
									strcpy_s(clients[id].name, name);
									clients[id].x = p_x;
									clients[id].y = p_y;
									clients[id]._max_hp = p_m_hp;
									clients[id]._hp = p_hp;
									clients[id]._level = p_level;
									clients[id]._max_exp = p_m_exp;
									clients[id]._exp = p_exp;
									clients[id]._atk = p_atk;
									clients[id]._def = p_def;
									clients[id]._job_no = p_job;
									clients[id]._gold = p_gold;
									printf("%d: %ls %d %d\n", i + 1, p_id, p_x, p_y);
									return true;
								}
								else
								{
									return false;
								}
							}
							else
								break;
						}
					}

					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstmt);
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}

					SQLDisconnect(hdbc);
				}
				else cout << "ODBC Connected Failed" << endl;
				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}

	return false;
}  

//플레이어 데이터 저장
bool DB_save(int id)
{
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0;
	SQLRETURN retcode;

	char temp[BUFSIZE];
	sprintf_s(temp, sizeof(temp), "EXEC select_save %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s", clients[id].x, 
		clients[id].y, clients[id]._max_hp, clients[id]._hp, clients[id]._level, clients[id]._max_exp, clients[id]._exp, clients[id]._atk, clients[id]._def, clients[id]._gold, clients[id]._job_no, clients[id].name);
	wchar_t* exec;
	int str_size = MultiByteToWideChar(CP_ACP, 0, temp, -1, NULL, NULL);
	exec = new WCHAR[str_size];
	MultiByteToWideChar(CP_ACP, 0, temp, sizeof(temp) + 1, exec, str_size);
	cout << temp << endl;
	
	SQLINTEGER p_x;
	SQLINTEGER p_y;
	SQLWCHAR p_id[NAME_LEN];
	SQLLEN cbP_ID = 0, cbP_X = 0, cbP_Y = 0;
	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2021_Class_ODBC", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					cout << "ODBC Connection Success" << endl;
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
					retcode = SQLExecDirect(hstmt, (SQLWCHAR*)exec, SQL_NTS);
					HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

						//// Bind columns 1, 2, and 3  
						//retcode = SQLBindCol(hstmt, 1, SQL_C_WCHAR, p_id, NAME_LEN, &cbP_ID);
						//retcode = SQLBindCol(hstmt, 2, SQL_C_LONG, &p_x, 100, &cbP_X);
						//retcode = SQLBindCol(hstmt, 3, SQL_C_LONG, &p_y, 100, &cbP_Y);


						// Fetch and print each row of data. On an error, display a message and exit.  
						for (int i = 0; ; i++) {
							retcode = SQLFetch(hstmt);
							if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
								show_err();
							if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
							{
								//replace wprintf with printf
								//%S with %ls
								//warning C4477: 'wprintf' : format string '%S' requires an argument of type 'char *'
								//but variadic argument 2 has type 'SQLWCHAR *'
								//wprintf(L"%d: %S %S %S\n", i + 1, sCustID, szName, szPhone);
								//strcpy_s(clients[id].name, (char*)p_id);
								
								return true;
							}
							else
								break;
						}
					}

					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstmt);
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}

					SQLDisconnect(hdbc);
				}
				else cout << "ODBC Connected Failed" << endl;
				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}

	return false;
}

//근처 객체 판별
bool is_near(int a, int b)
{
	if (RANGE < abs(clients[a].x - clients[b].x)) return false;
	if (RANGE < abs(clients[a].y - clients[b].y)) return false;
	return true;
}

//공격 범위
bool is_at_range(int at_rang, int a, int b)
{
	if (at_rang < abs(clients[a].x - clients[b].x)) return false;
	if (at_rang < abs(clients[a].y - clients[b].y)) return false;
	return true;
}

//npc 판별
bool is_npc(int id)
{
	return (id >= NPC_ID_START) && (id <= NPC_ID_END);
}

//플레이어 판별
bool is_player(int id)
{
	return (id >= 0) && (id < MAX_USER);
}

//새로운 id(인덱스) 할당
int get_id()
{
	static int g_id = 0;

	for (int i = 0; i < MAX_USER; ++i) {
		clients[i].state_lock.lock();
		if (ST_FREE == clients[i]._state) {
			clients[i]._state = ST_ACCEPT;
			clients[i].state_lock.unlock();
			return i;
		}
		else clients[i].state_lock.unlock();
	}
	cout << "Maximum Number of Clients Overflow!!\n";
	return -1;
}

//로그인 허용
void send_login_ok_packet(int c_id)
{
	sc_packet_login_ok packet;
	packet.id = c_id;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_OK;
	packet.x = clients[c_id].x;
	packet.y = clients[c_id].y;
	packet.maxhp = clients[c_id]._max_hp;
	packet.hp = clients[c_id]._hp;
	packet.level = clients[c_id]._level;
	packet.exp = clients[c_id]._exp;
	clients[c_id].do_send(sizeof(packet), &packet);
}

//로그인 실패
void send_login_fail_packet(int c_id)
{
	sc_packet_login_fail packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_FAIL;
	clients[c_id].do_send(sizeof(packet), &packet);
}

//이동
void send_move_packet(int _id, int target)
{
	sc_packet_move packet;
	packet.id = target;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_MOVE;
	packet.x = clients[target].x;
	packet.y = clients[target].y;
	packet.move_time = clients[target].last_move_time;
	clients[_id].do_send(sizeof(packet), &packet);
}

//오브젝트 제거
void send_remove_object(int c_id, int victim)
{
	sc_packet_remove_object packet;
	packet.id = victim;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_REMOVE_OBJECT;
	clients[c_id].do_send(sizeof(packet), &packet);
}

//오브젝트 생성
void send_put_object(int c_id, int target)
{
	sc_packet_put_object packet;
	packet.id = target;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_PUT_OBJECT;
	packet.x = clients[target].x;
	packet.y = clients[target].y;
	strcpy_s(packet.name, clients[target].name);
	packet.object_type = 0;
	clients[c_id].do_send(sizeof(packet), &packet);
}

//메세지 전송
void send_chat_packet(int user_id, int my_id, char* mess)
{
	sc_packet_chat packet;
	packet.id = my_id;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_CHAT;
	strcpy_s(packet.message, mess);
	clients[user_id].do_send(sizeof(packet), &packet);
}

//플레이어 데이터 변경
void send_status_packet(int c_id)
{
	sc_packet_status_change packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_STATUS_CHANGE;
	packet.maxhp = clients[c_id]._max_hp;
	packet.hp = clients[c_id]._hp;
	packet.level = clients[c_id]._level;
	packet.exp = clients[c_id]._exp;
	clients[c_id].do_send(sizeof(packet), &packet);
}

//해제
void Disconnect(int c_id)
{
	CLIENT& cl = clients[c_id];
	cl.vl.lock();
	unordered_set <int> my_vl = cl.viewlist;
	cl.vl.unlock();
	strcpy_s(clients[c_id].name," ");
	for (auto& other : my_vl) {
		CLIENT& target = clients[other];
		if (true == is_npc(target._id)) continue;
		if (ST_INGAME != target._state)
			continue;
		target.vl.lock();
		if (0 != target.viewlist.count(c_id)) {
			target.viewlist.erase(c_id);
			target.vl.unlock();
			send_remove_object(other, c_id);
		}
		else target.vl.unlock();
	}
	clients[c_id].state_lock.lock();
	clients[c_id]._state = ST_FREE;
	clients[c_id].state_lock.unlock();
	closesocket(clients[c_id]._socket);
	cout << "------------연결 종료------------" << endl;
}

//플레이어 이벤트 등록
void Player_Event(int target, int player_id, COMMAND type)
{
	Overlap* exp_over = new Overlap;
	exp_over->_op = type;
	exp_over->_target = player_id;
	PostQueuedCompletionStatus(g_h_iocp, 1, target, &exp_over->_wsa_over);
}

//npc 이벤트 등록
void Npc_Event(int NPC_id, int target_id, COMMAND type)
{
	Overlap* exp_over = new Overlap;
	exp_over->_op = type;
	exp_over->_target = target_id;
	PostQueuedCompletionStatus(g_h_iocp, 1, NPC_id, &exp_over->_wsa_over);
}

//타이머 큐 등록
void Timer_Event(int npc_id, int user_id, EVENT_TYPE ev, std::chrono::milliseconds ms)
{
	timer_ev order;
	order.this_id = npc_id;
	order.target_id = user_id;
	order.order = ev;
	order.start_t = chrono::system_clock::now() + ms;
	timer_q.push(order);
}

//패킷 판별
void process_packet(int client_id, unsigned char* p)
{
	unsigned char packet_type = p[1];
	CLIENT& cl = clients[client_id];

	switch (packet_type) {
	case CS_PACKET_LOGIN: {
		cs_packet_login* packet = reinterpret_cast<cs_packet_login*>(p);
		
		CLIENT& cl = clients[client_id];
		cl.state_lock.lock();
		cl._state = ST_INGAME;
		cl.state_lock.unlock();

		char name[MAX_NAME_SIZE];
		strcpy_s(name, packet->name);
		bool con = false;
		for (int i = 0; i < MAX_USER; ++i) {
			clients[i].state_lock.lock();
			if (ST_INGAME == clients[i]._state){
				if (strcmp(name, clients[i].name) == 0) {
					send_login_fail_packet(client_id);
					cout << name << " 로그인 실패" << endl;
					con = true;
					clients[i].state_lock.unlock();
					break;
				}
		      }
			clients[i].state_lock.unlock();
		}
		if (con == true) break;
		if (DB_odbc(client_id, name) == true)
		{
			send_login_ok_packet(client_id);
			cout << name << " 로그인 성공" << endl;
		}
		else {
			strcpy_s(cl.name, name);
			cl.x = 1000;
			cl.y = 0;
			cl._level = 1;
			cl._gold = 0;
			cl._max_hp = 100;
			cl._hp = 100;
			cl._max_exp = 100;
			cl._exp = 0;
			cl._job_no = 1;
			cl._atk = 10;
			cl._def = 10;
			//DB_save(client_id);
			send_login_ok_packet(client_id);
			cout << name << " 로그인 성공" << endl;
		}

		// 새로 접속한 플레이어의 정보를 주위 플레이어에게 보낸다
		for (auto& other : clients) {
			if (true == is_npc(other._id)) continue;
			if (other._id == client_id) continue;
			other.state_lock.lock();
			if (ST_INGAME != other._state) {
				other.state_lock.unlock();
				continue;
			}
			else other.state_lock.unlock();

			if (false == is_near(other._id, client_id))
				continue;

			other.vl.lock();
			other.viewlist.insert(client_id);
			other.vl.unlock();
			sc_packet_put_object packet;
			packet.id = client_id;
			strcpy_s(packet.name, cl.name);
			packet.object_type = 0;
			packet.size = sizeof(packet);
			packet.type = SC_PACKET_PUT_OBJECT;
			packet.x = cl.x;
			packet.y = cl.y;
			other.do_send(sizeof(packet), &packet);
		}

		// 새로 접속한 플레이어에게 주위 객체 정보를 보낸다
		for (auto& other : clients) {
			if (other._id == client_id) continue;
			other.state_lock.lock();
			if (ST_INGAME != other._state) {
				other.state_lock.unlock();
				continue;
			}
			else other.state_lock.unlock();

			if (false == is_near(other._id, client_id))
				continue;

			clients[client_id].vl.lock();
			clients[client_id].viewlist.insert(other._id);
			clients[client_id].vl.unlock();

			sc_packet_put_object packet;
			packet.id = other._id;
			strcpy_s(packet.name, other.name);
			packet.object_type = 0;
			packet.size = sizeof(packet);
			packet.type = SC_PACKET_PUT_OBJECT;
			packet.x = other.x;
			packet.y = other.y;
			cl.do_send(sizeof(packet), &packet);
		}
		break;
	}
	case CS_PACKET_MOVE: {
		cs_packet_move* packet = reinterpret_cast<cs_packet_move*>(p);
		cl.last_move_time = packet->move_time;
		
		int x = cl.x;
		int y = cl.y;

		switch (packet->direction) {
		case 0: if (y > 0) y--; break;
		case 1: if (y < (WORLD_HEIGHT - 1)) y++; break;
		case 2: if (x > 0) x--; break;
		case 3: if (x < (WORLD_WIDTH - 1)) x++; break;
		default:
			cout << "Invalid move in client " << client_id << endl;
			exit(-1);
		}
		
		cl.x = x;
		cl.y = y;

		unordered_set <int> near_list;
		for (auto& other : clients) {
			if (other._id == client_id)
				continue;
			if (ST_INGAME != other._state)
				continue;
			if (false == is_near(client_id, other._id))
				continue;
			if (true == is_npc(other._id)) {
				Player_Event(other._id, cl._id, OP_PLAYER_MOVE);
			}
			near_list.insert(other._id);
		}

		send_move_packet(cl._id, cl._id);

		cl.vl.lock();
		unordered_set <int> my_vl{ cl.viewlist };
		cl.vl.unlock();

		//새로 시야에 들어온 플레이어
		for (auto other_id : near_list) {
			if (0 == my_vl.count(other_id)) {
				cl.vl.lock();
				cl.viewlist.insert(other_id);
				cl.vl.unlock();
				send_put_object(cl._id, other_id);

				if (true == is_npc(other_id)) {
					/*   if (clients[other_id]._is_active != true) {
						  clients[other_id]._is_active = true;
						  timer_ev order;
						  order.this_id = other_id;
						  order.start_t = chrono::system_clock::now() + 1000ms;
						  timer_q.push(order);
					   }
					*/   continue;
				}

				clients[other_id].vl.lock();
				if (0 == clients[other_id].viewlist.count(cl._id)) {
					clients[other_id].viewlist.insert(cl._id);
					clients[other_id].vl.unlock();
					send_put_object(other_id, cl._id);
				}
				else {
					clients[other_id].vl.unlock();
					send_move_packet(other_id, cl._id);
				}
			}
			//시야에 존재하는 플레이어
			else {
				if (true == is_npc(other_id)) {
					/*if (clients[other_id]._is_active != true) {
					   clients[other_id]._is_active = true;
					   timer_ev order;
					   order.this_id = other_id;
					   order.start_t = chrono::system_clock::now() + 1000ms;
					   timer_q.push(order);
					}
					*/continue;
				}

				clients[other_id].vl.lock();
				if (0 != clients[other_id].viewlist.count(cl._id)) {
					clients[other_id].vl.unlock();
					send_move_packet(other_id, cl._id);
				}
				else {
					clients[other_id].viewlist.insert(cl._id);
					clients[other_id].vl.unlock();
					send_put_object(other_id, cl._id);
				}
			}
		}
		// 시야에서 벗어난 플레이어 
		for (auto other_id : my_vl) {
			if (0 == near_list.count(other_id)) {
				cl.vl.lock();
				cl.viewlist.erase(other_id);
				cl.vl.unlock();
				send_remove_object(cl._id, other_id);

				if (true == is_npc(other_id)) continue;

				clients[other_id].vl.lock();
				if (0 != clients[other_id].viewlist.count(cl._id)) {
					clients[other_id].viewlist.erase(cl._id);
					clients[other_id].vl.unlock();
					send_remove_object(other_id, cl._id);
				}
				else clients[other_id].vl.unlock();
			}
		}
		break;
	}
	case CS_PACKET_ATTACK: {
		cs_packet_attack* packet = reinterpret_cast<cs_packet_attack*>(p);
		cout << "attack" << endl;
		unordered_set <int> near_list;
		for (auto& other : clients) {
			if (other._id == client_id)
				continue;
			if (ST_INGAME != other._state)
				continue;
			if (false == is_at_range(cl._at_range, client_id, other._id))
				continue;
			if (true == is_npc(other._id)) {
				Player_Event(other._id, cl._id, OP_PLAYER_ATTACK);
			}
			near_list.insert(other._id);
		}

		send_move_packet(cl._id, cl._id);
		break;
	}
	case CS_PACKET_CHAT:
		break;
	case CS_PACKET_TELEPORT:
		break;
	default: 
		printf("Unknown PACKET type\n");

	}
}

//워크 쓰레드
void worker_thread()
{
	while(1) {
		DWORD num_byte;
		LONG64 iocp_key;
		WSAOVERLAPPED* p_over;
		BOOL ret = GetQueuedCompletionStatus(g_h_iocp, &num_byte, (PULONG_PTR)&iocp_key, &p_over, INFINITE);
	
		int client_id = static_cast<int>(iocp_key);
		Overlap* exp_over = reinterpret_cast<Overlap*>(p_over);
		if (FALSE == ret) {
			int err_no = WSAGetLastError();
			cout << "GQCS Error : ";
			error_display(err_no);
			cout << endl;
			Disconnect(client_id);
			if (exp_over->_op == OP_SEND)
				delete exp_over;
			continue;
		}

		switch (exp_over->_op) {
		case OP_RECV: {
			if (num_byte == 0) {
				cout << "연결종료" << endl;
				DB_save(client_id);
				Disconnect(client_id);
				continue;
			}
			CLIENT& cl = clients[client_id];
			int remain_data = num_byte + cl._prev_size;
			unsigned char* packet_start = exp_over->_net_buf;
			int packet_size = packet_start[0];

			while (packet_size <= remain_data) {
				process_packet(client_id, packet_start);
				remain_data -= packet_size;
				packet_start += packet_size;
				if (remain_data > 0) packet_size = packet_start[0];
				else break;
			}

			if (0 < remain_data) {
				cl._prev_size = remain_data;
				memcpy(&exp_over->_net_buf, packet_start, remain_data);
			}
			cl.do_recv();
			break;
		}
		case OP_SEND: {
			if (num_byte != exp_over->_wsa_buf.len) {
				cout << "send 에러" << endl;
				Disconnect(client_id);
			}
			delete exp_over;
			break;
		}
		case OP_ACCEPT: {
			cout << "Accept Completed.\n";
			SOCKET c_socket = *(reinterpret_cast<SOCKET*>(exp_over->_net_buf));
			int n_id = get_id();
			if (-1 == n_id) {
				cout << "user over.\n";
			}
			else {
				CLIENT& cl = clients[n_id];
				cl.x = rand() % WORLD_WIDTH;
				cl.y = rand() % WORLD_HEIGHT;
				//cl.x = 200;
				//cl.y = 200;

				cl._id = n_id;
				cl._prev_size = 0;
				cl._recv_over._op = OP_RECV;
				cl._recv_over._wsa_buf.buf = reinterpret_cast<char*>(cl._recv_over._net_buf);
				cl._recv_over._wsa_buf.len = sizeof(cl._recv_over._net_buf);
				ZeroMemory(&cl._recv_over._wsa_over, sizeof(cl._recv_over._wsa_over));
				cl._socket = c_socket;

				CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), g_h_iocp, n_id, 0);
				cl.do_recv();
			}

			ZeroMemory(&exp_over->_wsa_over, sizeof(exp_over->_wsa_over));
			c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
			*(reinterpret_cast<SOCKET*>(exp_over->_net_buf)) = c_socket;
			AcceptEx(sever_socket, c_socket, exp_over->_net_buf + 8, 0, sizeof(SOCKADDR_IN) + 16,
				sizeof(SOCKADDR_IN) + 16, NULL, &exp_over->_wsa_over);
		}
					  break;
		case OP_NPC_MOVE: {
			clients[client_id].lua_lock.lock();
			if (clients[client_id]._is_active == false) {
				clients[client_id].lua_lock.unlock();
				delete exp_over;
				break;
			}
			
			do_npc_move(client_id, exp_over->_target);

			clients[client_id].lua_lock.unlock();
			delete exp_over;
			break;
		}
		case OP_NPC_ATTACK: {
			clients[client_id].lua_lock.lock();
			if (clients[client_id]._is_active == false) {
				clients[client_id].lua_lock.unlock();
				delete exp_over;
				break;
			}
			
			do_npc_attack(client_id, exp_over->_target);



			clients[client_id].lua_lock.unlock();
			delete exp_over;
			break;
		}
		case OP_PLAYER_MOVE: {
			clients[client_id].lua_lock.lock();
			lua_State* L = clients[client_id].L;
			lua_getglobal(L, "event_player_move");
			lua_pushnumber(L, exp_over->_target);
			lua_pcall(L, 1, 0, 0);
			clients[client_id].lua_lock.unlock();
			delete exp_over;
			break;
		}
		case OP_PLAYER_ATTACK: {
			clients[client_id].lua_lock.lock();
			lua_State* L = clients[client_id].L;
			lua_getglobal(L, "event_player_attack");
			lua_pushnumber(L, exp_over->_target);
			lua_pcall(L, 1, 0, 0);
			clients[client_id].lua_lock.unlock();
			delete exp_over;
			break;
		}
		case OP_PLAYER_RE: {
			clients[client_id]._hp = clients[client_id]._max_hp;
			clients[client_id]._exp = clients[client_id]._exp / 2;
			send_status_packet(client_id);
			delete exp_over;
			break;
		}
		}
	}
}

//lua 메세지 전송
int API_Send_Msg(lua_State* L)
{
	//cout << "sendmess" << endl;
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);

	lua_pop(L, 4);

	send_chat_packet(user_id, my_id, mess);
	return 0;
}

//lua 공격 범위 판별
int API_at_range(lua_State* L)
{
	//cout << "is_at_near" << endl;
	int my_id = (int)lua_tointeger(L, -2);
	int user_id = (int)lua_tointeger(L, -1);
	
	lua_pop(L, 3);
	bool rt = is_at_range(clients[user_id]._at_range ,my_id, user_id);
	cout << rt << endl;
	lua_pushboolean(L, rt);
	return 1;
}

//lua 공격 범위 판별
int API_og_range(lua_State* L)
{
	//cout << "is_at_near" << endl;
	int my_id = (int)lua_tointeger(L, -2);
	int user_id = (int)lua_tointeger(L, -1);

	lua_pop(L, 3);
	bool rt = is_at_range(OG_RANGE, my_id, user_id);
	lua_pushboolean(L, rt);
	return 1;
}

//lua x 값 받아오기
int API_get_x(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = clients[user_id].x;
	lua_pushnumber(L, x);
	return 1;
}

//lua 공격력 값 받아오기
int API_get_power(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int power = clients[user_id]._atk;
	lua_pushnumber(L, power);
	return 1;
}

//lua y 값 받아오기
int API_get_y(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int y = clients[user_id].y;
	lua_pushnumber(L, y);
	return 1;
}

//lua y 값 받아오기
int API_get_t(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int t = clients[user_id]._type;
	lua_pushnumber(L, t);
	return 1;
}

//lua hp 값 받아오기
int API_get_hp(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int hp = clients[user_id]._hp;
	lua_pushnumber(L, hp);
	return 1;
}

//lua npc 이동
int API_move_npc(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -2);
	int user_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 3);
	if (clients[my_id]._is_active != true) {
		clients[my_id]._is_active = true;	
	}
	Timer_Event(my_id, user_id, NPC_MOVE, 1000ms);
	Timer_Event(my_id, user_id, NPC_MOVE, 2000ms);
	Timer_Event(my_id, user_id, NPC_MOVE, 3000ms);

	return 0;
}

//lua npc 공격
int API_attack_npc(lua_State* L)
{
	cout << "attack_npc" << endl;
	int my_id = (int)lua_tointeger(L, -2);
	int user_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 3);

	if (clients[my_id]._hp > 0) {
	if (clients[my_id]._is_active != true) {
		clients[my_id]._is_active = true;
		clients[my_id]._combat = COMBAT_DO;
	}

	clients[my_id]._hp -= clients[user_id]._atk;
	char buf[100];
	sprintf_s(buf, "player ATTACK!!!!\n%s -%d)", clients[my_id].name, clients[user_id]._atk);
	send_chat_packet(user_id, -1, buf);

	//clients[npc_id].state_lock.unlock();
	if (clients[my_id]._hp <= 0) {
		
		clients[my_id]._is_active = false;
		clients[my_id]._state = ST_FREE;
		clients[my_id]._combat = COMBAT_END;
		clients[user_id]._exp += (clients[my_id]._level * 10);
		if (clients[user_id]._exp >= clients[user_id]._max_exp)
		{
			int max_hp = clients[user_id]._max_hp * 2;
			clients[user_id]._max_hp = max_hp;
			clients[user_id]._hp = max_hp;
			clients[user_id]._max_exp = max_hp;
			clients[user_id]._max_exp = 0;
			clients[user_id]._atk = clients[user_id]._atk * 2;
			clients[user_id]._def = clients[user_id]._def * 2;
			char buf[100];
			sprintf_s(buf, "LEVEL UP!!!!!)");
			send_chat_packet(user_id, -1, buf);

		}
		else {
			char buf[100];
			sprintf_s(buf, "EXP UP!!!!!\n + %d)", clients[my_id]._level * 10);
			send_chat_packet(user_id, -1, buf);
		}
		send_status_packet(user_id);
		for (auto& other : clients) {
			if (true == is_npc(other._id)) continue;
			other.state_lock.lock();
			if (ST_INGAME != other._state) {
				other.state_lock.unlock();
				continue;
			}
			else other.state_lock.unlock();

			if (false == is_near(other._id, my_id))
				continue;
			send_remove_object(other._id, my_id);
		}
		
		

	}
	else{
		
		Timer_Event(my_id, user_id, NPC_ATTACK, 1000ms);

		}
	}
	else {
		clients[my_id]._is_active = false;
	}
	return 0;
}

//npc 초기화
void Initialize_NPC()
{
	cout << "로딩중.." << endl;
	for (int i = NPC_ID_START; i < NPC_ID_END / 2; ++i) {
		sprintf_s(clients[i].name, "GOBELIN [LV 1]");
		clients[i].x = rand() % WORLD_WIDTH;
		clients[i].y = rand() % (WORLD_HEIGHT / 2);
		clients[i]._id = i;
		clients[i]._state = ST_INGAME;
		clients[i]._type = 2;
		clients[i]._hp = 100;
		clients[i]._level = 1;
		clients[i]._atk = 10;
		clients[i]._at_range = 1;
		clients[i]._is_active = false;
		clients[i]._combat = COMBAT_END;
		clients[i]._count = 0;

		lua_State* L = clients[i].L = luaL_newstate();
		luaL_openlibs(L);
		int error = luaL_loadfile(L, "monster.lua") ||
			lua_pcall(L, 0, 0, 0);
		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 1, 0);
		lua_pop(L, 1);// eliminate set_uid from stack after call

		lua_register(L, "API_Send_Msg", API_Send_Msg);
		lua_register(L, "API_move_npc", API_move_npc);
		lua_register(L, "API_at_range", API_at_range);
		lua_register(L, "API_og_range", API_og_range);

		lua_register(L, "API_attack_npc", API_attack_npc);

		lua_register(L, "API_get_x", API_get_x);
		lua_register(L, "API_get_y", API_get_y);
		lua_register(L, "API_get_t", API_get_t);
		lua_register(L, "API_get_hp", API_get_hp);
		lua_register(L, "API_get_power", API_get_power);

	}
	for (int i = NPC_ID_END / 2; i < NPC_ID_END - 1; ++i) {
		sprintf_s(clients[i].name, "Ogre\n[LV 2]");
		clients[i].x = rand() % WORLD_WIDTH;
		clients[i].y = rand() % (WORLD_HEIGHT / 2) + (WORLD_HEIGHT / 3);
		clients[i]._id = i;
		clients[i]._state = ST_INGAME;
		clients[i]._type = 3;
		clients[i]._hp = 200;
		clients[i]._level = 2;
		clients[i]._atk = 20;
		clients[i]._at_range = 1;
		clients[i]._combat = COMBAT_END;

		clients[i]._is_active = false;
		clients[i]._count = 0;

		lua_State* L = clients[i].L = luaL_newstate();
		luaL_openlibs(L);
		int error = luaL_loadfile(L, "monster.lua") ||
			lua_pcall(L, 0, 0, 0);
		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 1, 0);
		lua_pop(L, 1);// eliminate set_uid from stack after call

		lua_register(L, "API_Send_Msg", API_Send_Msg);
		lua_register(L, "API_move_npc", API_move_npc);
		lua_register(L, "API_at_range", API_at_range);
		lua_register(L, "API_og_range", API_og_range);

		lua_register(L, "API_attack_npc", API_attack_npc);

		lua_register(L, "API_get_x", API_get_x);
		lua_register(L, "API_get_y", API_get_y);
		lua_register(L, "API_get_t", API_get_t);

		lua_register(L, "API_get_hp", API_get_hp);
		lua_register(L, "API_get_power", API_get_power);

	}

	sprintf_s(clients[NPC_ID_END].name, "DRAGON\n[LV 3]");
	clients[NPC_ID_END].x = 1000;
	clients[NPC_ID_END].y = 1900;
	clients[NPC_ID_END]._id = NPC_ID_END;
	clients[NPC_ID_END]._state = ST_INGAME;
	clients[NPC_ID_END]._type = 4;
	clients[NPC_ID_END]._hp = 300;
	clients[NPC_ID_END]._level = 3;
	clients[NPC_ID_END]._atk = 30;
	clients[NPC_ID_END]._at_range = 1;
	clients[NPC_ID_END]._is_active = false;
	clients[NPC_ID_END]._combat = COMBAT_END;

	clients[NPC_ID_END]._count = 0;

	lua_State* L = clients[NPC_ID_END].L = luaL_newstate();
	luaL_openlibs(L);
	int error = luaL_loadfile(L, "monster.lua") ||
		lua_pcall(L, 0, 0, 0);
	lua_getglobal(L, "set_uid");
	lua_pushnumber(L, NPC_ID_END);
	lua_pcall(L, 1, 1, 0);
	lua_pop(L, 1);// eliminate set_uid from stack after call

	lua_register(L, "API_Send_Msg", API_Send_Msg);
	lua_register(L, "API_move_npc", API_move_npc);
	lua_register(L, "API_at_range", API_at_range);
	lua_register(L, "API_og_range", API_og_range);

	lua_register(L, "API_attack_npc", API_attack_npc);

	lua_register(L, "API_get_x", API_get_x);
	lua_register(L, "API_get_y", API_get_y);
	lua_register(L, "API_get_t", API_get_t);

	lua_register(L, "API_get_hp", API_get_hp);
	lua_register(L, "API_get_power", API_get_power);

	cout << "npc로딩 완료" << endl;
}

// npc 이동 AI
void do_npc_move(int npc_id, int user_id)
{
	unordered_set <int> old_vl;
	unordered_set <int> new_vl;

	for (auto& obj : clients) {
		if (obj._state != ST_INGAME)
			continue;
		if (false == is_player(obj._id))
			continue;
		if (true == is_near(npc_id, obj._id))
			old_vl.insert(obj._id);
	}

	/*if (old_vl.empty()) {
	   clients[npc_id]._is_active = false;
	   return;
	}*/

	auto& x = clients[npc_id].x;
	auto& y = clients[npc_id].y;
	//clients[npc_id].state_lock.lock();
	if (clients[npc_id]._count < 3)
	{
		clients[npc_id]._count += 1;
		switch (rand() % 4) {
		case 0: if (y > 0) y--; break;
		case 1: if (y < (WORLD_HEIGHT - 1)) y++; break;
		case 2: if (x > 0) x--; break;
		case 3: if (x < (WORLD_WIDTH - 1)) x++; break;
		}
		//clients[npc_id].state_lock.unlock();
		if (clients[npc_id]._count >= 3) {
			clients[npc_id]._is_active = false;
			clients[npc_id]._count = 0;
		}
	}

	for (auto& obj : clients) {
		if (obj._state != ST_INGAME)
			continue;
		if (false == is_player(obj._id))
			continue;
		if (true == is_near(npc_id, obj._id))
			new_vl.insert(obj._id);
	}


	//if (new_vl.empty()) {
	//   clients[npc_id]._is_active = false;
	//   return;
	//}
	//else {
	//   timer_ev order;
	//   order.this_id = npc_id;
	//   order.start_t = chrono::system_clock::now() + 1000ms;
	//   timer_q.push(order);
	//}


	// 새로 시야에 들어온 플레이어
	for (auto pl : new_vl) {
		if (0 == old_vl.count(pl)) {
			clients[pl].vl.lock();
			clients[pl].viewlist.insert(npc_id);
			clients[pl].vl.unlock();
			send_put_object(pl, npc_id);
		}
		else {
			send_move_packet(pl, npc_id);
		}
	}
	// 시야에서 사라지는 경우
	for (auto pl : old_vl) {
		if (0 == new_vl.count(pl)) {
			clients[pl].vl.lock();
			clients[pl].viewlist.erase(npc_id);
			clients[pl].vl.unlock();
			send_remove_object(pl, npc_id);
		}
	}

}

// npc 공격 AI
void do_npc_attack(int npc_id, int user_id)
{
	unordered_set <int> old_vl;
	unordered_set <int> new_vl;
	
	cout << "do_npc_attack" << endl;

	for (auto& obj : clients) {
		if (obj._state != ST_INGAME)
			continue;
		if (false == is_player(obj._id))
			continue;
		if (true == is_at_range(clients[npc_id]._at_range ,npc_id, obj._id))
			old_vl.insert(obj._id);
	}

	/*if (old_vl.empty()) {
	   clients[npc_id]._is_active = false;
	   return;
	}*/

	auto& x = clients[npc_id].x;
	auto& y = clients[npc_id].y;
	//clients[npc_id].state_lock.lock();
	
	if (old_vl.empty())
	{
		clients[npc_id]._combat = COMBAT_END;
		clients[npc_id]._is_active = false;
	}
	else 
	{
		if (clients[npc_id]._hp > 0)
		{

			for (auto pl : old_vl) {
				clients[pl].vl.lock();
				clients[pl]._hp -= clients[npc_id]._atk;
				cout << "atk" << clients[npc_id]._atk << endl;
				clients[pl].vl.unlock();
				string s = to_string(clients[npc_id]._atk * -1);
				const char* mess = s.c_str();
				//cout << "mess" << mess << endl;
				send_chat_packet(pl, pl, (char*)mess);
				char buf[100];
				sprintf_s(buf, "%s ATTACK!!!!\nPLAYER -%d)", clients[npc_id].name, clients[npc_id]._atk);
				send_chat_packet(pl, -1, buf);
				send_status_packet(pl);
				if(clients[pl]._hp <= 0)
				{
					Player_Event(pl, pl, OP_PLAYER_RE);
				}
			}
			
			Timer_Event(npc_id, user_id, NPC_ATTACK, 1000ms);

			
		}
		else
			clients[npc_id]._is_active = false;

	}
	return;	
}

//타이머
void ev_timer() {

	while (true) {
		timer_ev order;
		timer_q.try_pop(order);
		//auto t = order.start_t - chrono::system_clock::now();
		int npc_id = order.this_id;
		if (false == is_npc(npc_id)) continue;
		if (clients[npc_id]._state != ST_INGAME) continue;
		if (clients[npc_id]._is_active == false) continue;
		if (order.start_t <= chrono::system_clock::now()) {
			if (order.order == NPC_MOVE) {
				Npc_Event(npc_id, order.target_id, OP_NPC_MOVE);
				this_thread::sleep_for(50ms);
			}
			else if (order.order == NPC_ATTACK)
			{
				cout << "atk_time" << endl;
				if (clients[npc_id]._hp > 0 && clients[npc_id]._combat == COMBAT_DO) {
					Npc_Event(npc_id, order.target_id, OP_NPC_ATTACK);
					this_thread::sleep_for(50ms);
				}
				else
				  clients[npc_id]._is_active = false;
			}

		}
		else {
			timer_q.push(order);
			this_thread::sleep_for(10ms);

		}


	}
}

int main()
{
	wcout.imbue(locale("korean"));
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	sever_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(sever_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(sever_socket, SOMAXCONN);

	g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(sever_socket), g_h_iocp, 0, 0);

	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	char   accept_buf[sizeof(SOCKADDR_IN) * 2 + 32 + 100];
	Overlap   accept_ex;
	*(reinterpret_cast<SOCKET*>(&accept_ex._net_buf)) = c_socket;
	ZeroMemory(&accept_ex._wsa_over, sizeof(accept_ex._wsa_over));
	accept_ex._op = OP_ACCEPT;

	AcceptEx(sever_socket, c_socket, accept_buf, 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, NULL, &accept_ex._wsa_over);
	
	for (int i = 0; i < MAX_USER; ++i)
		clients[i]._id = i;

	Initialize_NPC();

	vector <thread> worker_threads;
	
	thread timer_thread{ ev_timer };
	for (int i = 0; i < 10; ++i)
		worker_threads.emplace_back(worker_thread);
	for (auto& th : worker_threads)
		th.join();

	timer_thread.join();
	for (auto& cl : clients) {
		if (ST_INGAME == cl._state)
			Disconnect(cl._id);
	}
	closesocket(sever_socket);
	WSACleanup();
}



//----------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------

