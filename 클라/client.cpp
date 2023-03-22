#define SFML_STATIC 1
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <chrono>
using namespace std;

#ifdef _DEBUG
#pragma comment (lib, "lib/sfml-graphics-s-d.lib")
#pragma comment (lib, "lib/sfml-window-s-d.lib")
#pragma comment (lib, "lib/sfml-system-s-d.lib")
#pragma comment (lib, "lib/sfml-network-s-d.lib")
#else
#pragma comment (lib, "lib/sfml-graphics-s.lib")
#pragma comment (lib, "lib/sfml-window-s.lib")
#pragma comment (lib, "lib/sfml-system-s.lib")
#pragma comment (lib, "lib/sfml-network-s.lib")
#endif
#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "ws2_32.lib")

#include "protocol.h"

sf::TcpSocket socket;

constexpr auto BUF_SIZE = 256;
constexpr auto SCREEN_WIDTH = 20;
constexpr auto SCREEN_HEIGHT = 20;

constexpr auto TILE_WIDTH = 65;
constexpr auto WINDOW_WIDTH = TILE_WIDTH * SCREEN_WIDTH + 10;   // size of window
constexpr auto WINDOW_HEIGHT = TILE_WIDTH * SCREEN_WIDTH + 10;
//constexpr auto BUF_SIZE = MAX_BUFFER;

bool g_login_ok;
string g_name;


int g_myid;
int* g_m_hp;
int* g_hp;
int* g_level;
int* g_exp;

int g_x_origin;
int g_y_origin;

char g_chat_buf[BUF_SIZE];
sf::RenderWindow* g_window;
sf::Font g_font;
void send_login_packet();

class OBJECT {
private:
	bool m_showing;
	sf::Sprite m_sprite;
	sf::Text m_name;
	sf::Text m_chat;
	chrono::system_clock::time_point m_mess_end_time;
public:
	int  _id; // array의 저장된 인덱스 <- 서버 관리용
	int m_x, m_y;
	int _type;
	int _max_hp; // 최대 체력
	int _hp; // 체력
	int _level; // 레벨
	int _max_exp;
	int _exp;
	int _atk = 10; // 공격력
	int _def;
	int _job_no;
	int _gold;
	int _at_range = 1; // 기본 공격 범위
	int _sk_range; // 스킬 범위

	OBJECT(sf::Texture& t, int x, int y, int x2, int y2) {
		m_showing = false;
		m_sprite.setTexture(t);
		m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
		set_name("NONAME");
		m_mess_end_time = chrono::system_clock::now();
	}
	OBJECT() {
		m_showing = false;
	}
	void show()
	{
		m_showing = true;
	}
	void hide()
	{
		m_showing = false;
	}

	void a_move(int x, int y) {
		m_sprite.setPosition((float)x, (float)y);
	}

	void a_draw() {
		g_window->draw(m_sprite);
	}

	void move(int x, int y) {
		m_x = x;
		m_y = y;
	}
	void draw() {
		if (false == m_showing) return;
		float rx = (m_x - g_x_origin) * 65.0f + 8;
		float ry = (m_y - g_y_origin) * 65.0f + 8;
		m_sprite.setPosition(rx, ry);
		g_window->draw(m_sprite);
		sf::RectangleShape
			rectangle(sf::Vector2f(500, 200));
		rectangle.setFillColor(sf::Color(0, 0, 0, 200));
		g_window->draw(rectangle);

		if (m_mess_end_time < chrono::system_clock::now()) {
			m_name.setPosition(rx - 10, ry - 20);
			g_window->draw(m_name);
			// 화면에 쓸 내용
			string s = "HP : " + to_string(*g_hp) + "/" + to_string(*g_m_hp) + "\nLEVEL :" + to_string(*g_level) + "\nEXP :" + to_string(*g_exp);
			//cout << s;
			sf::Text text(s, g_font, 30);
			text.setFillColor(sf::Color::Green); //글씨 색깔
			g_window->draw(text);

		}
		else {
			m_chat.setPosition(rx - 10, ry - 20);
			g_window->draw(m_chat);
			string s = "HP : " + to_string(*g_hp) + "/" + to_string(*g_m_hp) + "\nLEVEL :" + to_string(*g_level) + "\nEXP :" + to_string(*g_exp);
			//cout << s;
			sf::Text text(s, g_font, 30);
			text.setFillColor(sf::Color::Green); //글씨 색깔
			g_window->draw(text);
		}
	}
	void set_name(const char str[]) {
		m_name.setFont(g_font);
		m_name.setString(str);
		m_name.setFillColor(sf::Color(255, 255, 0));
		m_name.setStyle(sf::Text::Bold);
	}
	void set_chat(const char str[]) {
		m_chat.setFont(g_font);
		m_chat.setString(str);
		m_chat.setFillColor(sf::Color(255, 0, 0));
		m_chat.setStyle(sf::Text::Bold);
		m_mess_end_time = chrono::system_clock::now() + chrono::seconds(3);


	}
};
OBJECT avatar;

OBJECT players[MAX_USER + MAX_NPC];

OBJECT Green_tile;
OBJECT wroad_tile;
OBJECT soil_tile;
OBJECT water_tile;
OBJECT rock_tile;


sf::Texture* board;
sf::Texture* pieces;

void client_initialize()
{
	board = new sf::Texture;
	pieces = new sf::Texture;
	if (false == g_font.loadFromFile("cour.ttf")) {
		cout << "Font Loading Error!\n";
		while (true);
	}
	board->loadFromFile("chessmap.png");
	//board->loadFromFile("chessmap.bmp");
	//board->loadFromFile("Overworld.png");
	pieces->loadFromFile("chess2.png");
	Green_tile = OBJECT{ *board, 0, 0, TILE_WIDTH, TILE_WIDTH };
	wroad_tile = OBJECT{ *board, 5, 70, TILE_WIDTH, TILE_WIDTH };
	soil_tile = OBJECT{ *board, 5, 140, TILE_WIDTH, TILE_WIDTH };
	water_tile = OBJECT{ *board, 5, 210, TILE_WIDTH, TILE_WIDTH };
	rock_tile = OBJECT{ *board, 5, 280, TILE_WIDTH, TILE_WIDTH };

	avatar = OBJECT{ *pieces, 128, 0, 64, 64 };
	for (int i = 0; i <= NPC_ID_END; ++i) {
		if (i < MAX_USER) { // PLAYER
			players[i] = OBJECT{ *pieces, 128, 0, 64, 64 };

		}
		else if (MAX_USER <= i && i < (NPC_ID_END / 2))// 고블린
		{  // NPC
			//cout << "고블린" << endl;
			players[i] = OBJECT{ *pieces, 0, 0, 64, 64 };
		}
		else if ((NPC_ID_END / 2) <= i && i < NPC_ID_END) // 오거
		{  // NPC
			//cout << " 오거" << endl;
			players[i] = OBJECT{ *pieces, 64, 0, 64, 64 };
		}
		else
		{   // NPC
			//cout << "드래곤" << endl;
			players[i] = OBJECT{ *pieces, 320, 0, 64, 64 };
		}
	}
}

void client_finish()
{
	delete board;
	delete pieces;
}

void ProcessPacket(char* ptr)
{
	static bool first_time = true;
	switch (ptr[1])
	{
	case SC_PACKET_LOGIN_OK:
	{
		printf("완료");
		g_login_ok = true;
		sc_packet_login_ok* packet = reinterpret_cast<sc_packet_login_ok*>(ptr);
		g_myid = packet->id;
		avatar.m_x = packet->x;
		avatar.m_y = packet->y;
		g_x_origin = packet->x - SCREEN_WIDTH / 2;
		g_y_origin = packet->y - SCREEN_WIDTH / 2;
		avatar._max_hp = packet->maxhp;
		g_m_hp = &avatar._max_hp;
		avatar._hp = packet->hp;
		g_hp = &avatar._hp;
		avatar._level = packet->level;
		g_level = &avatar._level;
		avatar._exp = packet->exp;
		g_exp = &avatar._exp;
		avatar.move(packet->x, packet->y);
		avatar.show();
	}
	break;
	case SC_PACKET_LOGIN_FAIL:
	{

		printf("아이디 다시 입력");
		send_login_packet();
	}
	break;

	case SC_PACKET_PUT_OBJECT:
	{
		sc_packet_put_object* my_packet = reinterpret_cast<sc_packet_put_object*>(ptr);
		int id = my_packet->id;

		if (id < MAX_USER) { // PLAYER
			players[id]._type = 1;
			players[id].set_name(my_packet->name);
			players[id].move(my_packet->x, my_packet->y);
			players[id].show();
		}
		else {  // NPC
			players[id]._type = 2;
			players[id].set_name(my_packet->name);
			players[id].move(my_packet->x, my_packet->y);
			players[id].show();
		}
		break;
	}
	case SC_PACKET_MOVE:
	{
		sc_packet_move* my_packet = reinterpret_cast<sc_packet_move*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_x_origin = my_packet->x - SCREEN_WIDTH / 2;
			g_y_origin = my_packet->y - SCREEN_WIDTH / 2;
		}
		else if (other_id < MAX_USER) {
			players[other_id].move(my_packet->x, my_packet->y);
		}
		else {
			players[other_id].move(my_packet->x, my_packet->y);
		}
		break;
	}

	case SC_PACKET_REMOVE_OBJECT:
	{
		sc_packet_remove_object* my_packet = reinterpret_cast<sc_packet_remove_object*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.hide();
		}
		else if (other_id < MAX_USER) {
			players[other_id].hide();
		}
		else {
			players[other_id].hide();
		}
		break;
	}

	case SC_PACKET_CHAT:
	{
		sc_packet_chat* my_packet = reinterpret_cast<sc_packet_chat*>(ptr);
		int other_id = my_packet->id;

		if (other_id == -1) {
			strcpy_s(g_chat_buf, my_packet->message);
		}
		else if (other_id == g_myid) {
			avatar.set_chat(my_packet->message);
		}
		else if (other_id < MAX_USER) {
			players[other_id].set_chat(my_packet->message);
		}
		else {
			players[other_id].set_chat(my_packet->message);
		}
		break;
	}
	case SC_PACKET_STATUS_CHANGE:
	{
		sc_packet_status_change* packet = reinterpret_cast<sc_packet_status_change*>(ptr);
		avatar._max_hp = packet->maxhp;
		avatar._hp = packet->hp;
		avatar._level = packet->level;
		avatar._exp = packet->exp;
		break;
	}

	default:
		printf("Unknown PACKET type [%d]\n", ptr[1]);
	}
}

void process_data(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

bool client_main()
{
	char net_buf[BUF_SIZE];
	size_t	received;

	auto recv_result = socket.receive(net_buf, BUF_SIZE, received);
	if (recv_result == sf::Socket::Error)
	{
		wcout << L"Recv 에러!";
		while (true);
	}
	if (recv_result == sf::Socket::Disconnected)
	{
		wcout << L"서버 접속 종료.\n";
		return false;
	}
	if (recv_result != sf::Socket::NotReady)
		if (received > 0) process_data(net_buf, received);

	for (int i = 0; i < SCREEN_WIDTH; ++i) {
		for (int j = 0; j < SCREEN_HEIGHT; ++j)
		{
			int tile_x = i + g_x_origin;
			int tile_y = j + g_y_origin;
			if ((tile_x < 0) || (tile_y < 0)) continue;
			if (tile_x > 1000 && tile_x < 1010)
			{
					soil_tile.a_move(TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
					soil_tile.a_draw();
			}
			else
			{
				if (tile_y < 500) {
					Green_tile.a_move(TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
					Green_tile.a_draw();
				}
				else if (tile_y >= 500 && tile_y < 600)
				{
					water_tile.a_move(TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
					water_tile.a_draw();

				}
				else if (tile_y >= 600 && tile_y < 700)
				{
					Green_tile.a_move(TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
					Green_tile.a_draw();

				}
				else if (tile_y >= 700 && tile_y < 800)
				{
					soil_tile.a_move(TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
					soil_tile.a_draw();

				}
				else {
					if ((((tile_x / 3) + (tile_y / 3)) % 2) == 1) {
						Green_tile.a_move(TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
						Green_tile.a_draw();
					}
					else
					{
						Green_tile.a_move(TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
						Green_tile.a_draw();

					}
				}
			}
		}
	}
	avatar.draw();
	for (auto& pl : players) pl.draw();
	sf::Text text;
	text.setFont(g_font);
	text.setPosition(0, 100);
	text.setString(g_chat_buf);
	text.setFillColor(sf::Color::Red); //글씨 색깔
	g_window->draw(text);


	return true;
}

void send_move_packet(char dr)
{
	cs_packet_move packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_MOVE;
	packet.direction = dr;
	size_t sent = 0;
	socket.send(&packet, sizeof(packet), sent);
}

void send_attack_packet()
{
	cs_packet_attack packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_ATTACK;
	size_t sent = 0;
	socket.send(&packet, sizeof(packet), sent);
}


void send_login_packet()
{
	printf("아이디 입력 : ");
	scanf_s("%s", &g_name, MAX_NAME_SIZE);
	cs_packet_login packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_LOGIN;
	strcpy_s(packet.name, g_name.c_str());
	size_t sent = 0;
	socket.send(&packet, sizeof(packet), sent);
	printf("\n%s 로그인 중\n", packet.name);
}


int main()
{
	wcout.imbue(locale("korean"));
	sf::Socket::Status status = socket.connect("127.0.0.1", SERVER_PORT);


	socket.setBlocking(false);

	if (status != sf::Socket::Done) {
		wcout << L"서버와 연결할 수 없습니다.\n";
		while (true);
	}

	client_initialize();
	string name{ "PL" };

	g_login_ok = false;
	send_login_packet();


	char net_buf[BUF_SIZE];
	size_t	received;

	name = g_name;
	avatar.set_name(name.c_str());


	avatar.set_name(name.c_str());
	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2D CLIENT");
	g_window = &window;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::KeyPressed) {
				int direction = -1;
				switch (event.key.code) {
				case sf::Keyboard::Left:
					direction = 2;
					break;
				case sf::Keyboard::Right:
					direction = 3;
					break;
				case sf::Keyboard::Up:
					direction = 0;
					break;
				case sf::Keyboard::Down:
					direction = 1;
					break;
				case sf::Keyboard::Escape:
					window.close();
					break;

				case sf::Keyboard::Space:
					direction = 4;
					break;

				}
				if (-1 != direction)
				{
					if (4 == direction)
					{
						send_attack_packet();
					}
					else
						send_move_packet(direction);
				}
			}
		}

		window.clear();
		if (false == client_main())
			window.close();
		window.display();
	}
	client_finish();

	return 0;
}