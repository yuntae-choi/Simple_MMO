#include "pch.h"
#include "CLIENT.h"
#include "Inven.h"
#include "Item.h"

CInven::CInven()
	: m_pPlayer(nullptr), m_iSize(5)
{
}


CInven::~CInven()
{
	Release();
}

void CInven::Initialize()
{
	m_vecInven.reserve(m_iSize);
}

void CInven::Progress()
{
	int iSelect = 0;

	while (true)
	{
		system("cls");
		//m_pPlayer->Render();
		cout << "1.장착 2.해제 3.나가기" << endl;
		cout << "=================================================" << endl;
		cout << "입력: ";
		cin >> iSelect;

		switch (iSelect)
		{
		case 1:
			Equip_Item();
			break;
		case 2:
			UnEquip_Item();
			break;
		case 3:
			return;
		default:
			continue;
		}
	}
}

void CInven::Render()
{
	for (size_t i = 0; i < m_vecInven.size(); ++i)
	{
		cout << i + 1 << ") " << endl;
		m_vecInven[i]->Render();
	}
}

void CInven::Release()
{
	//for (size_t i = 0; i < m_vecInven.size(); ++i)
	//	SAFE_DELETE(m_vecInven[i]);

	//for_each(m_vecInven.begin(), m_vecInven.end(), Safe_Delete<CObj*>);
	m_vecInven.clear();
}

bool CInven::Buy_Item(CObj* _pItem)
{
	if (m_vecInven.size() >= m_iSize)
		return false;

	//m_vecInven.push_back(_pItem);

	CItem* pItem = dynamic_cast<CItem*>(_pItem);
	m_vecInven.push_back(new CItem(*pItem));

	return true;
}

bool CInven::Sell_Item(int _iIdx, int* _pGold)
{
	if ((size_t)_iIdx >= m_vecInven.size())
		return false;

	if (STATE_EQUIP == dynamic_cast<CItem*>(m_vecInven[_iIdx])->Get_State())
		return false;

	vector<CObj*>::iterator		iter = m_vecInven.begin();
	iter += _iIdx;

	*_pGold = (*iter)->Get_Gold() >> 1;

	//SAFE_DELETE(*iter);
	m_vecInven.erase(iter);

	return true;

}

void CInven::Equip_Item()
{
	int iSelect = 0;

	while (true)
	{
		system("cls");
		//m_pPlayer->Render();
		Render();
		cout << "0) 나가기" << endl;
		cout << "=================================================" << endl;
		cout << "입력: ";
		cin >> iSelect;
		--iSelect;

		if (0 > iSelect)
			return;

		if (m_vecInven.size() <= (size_t)iSelect)
			continue;

		dynamic_cast<CLIENT*>(m_pPlayer)->Equip_Item(m_vecInven[iSelect]);
	}
}

void CInven::UnEquip_Item()
{
	int iSelect = 0;

	while (true)
	{
		system("cls");
		//m_pPlayer->Render();
		cout << "0) 나가기" << endl;
		cout << "=================================================" << endl;
		cout << "입력: ";
		cin >> iSelect;
		--iSelect;

		if (0 > iSelect)
			return;

		dynamic_cast<CLIENT*>(m_pPlayer)->UnEquip_Item(iSelect);
	}
}

INFO& CInven::Save_Info(size_t _iIdx)
{
	return dynamic_cast<CItem*>(m_vecInven[_iIdx])->Save_Data();
}

TYPE& CInven::Save_Type(size_t _iIdx)
{
	return dynamic_cast<CItem*>(m_vecInven[_iIdx])->Get_Type();
}

//STATE& CInven::Save_State(size_t _iIdx)
//{
//	return dynamic_cast<CItem*>(m_vecInven[_iIdx])->Get_State();
//}

void CInven::Load_Item(CObj* _pItem)
{
	m_vecInven.push_back(_pItem);
}
