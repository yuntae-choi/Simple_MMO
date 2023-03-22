#pragma once


#ifndef __INVEN_H__
#define __INVEN_H__

class CObj;
class CInven
{
public:
	CInven();
	~CInven();

public:
	void Initialize();
	void Progress();
	void Render();
	void Release();

public:
	bool Buy_Item(CObj* _pItem);
	bool Sell_Item(int _iIdx, int* _pGold);

public:
	void Equip_Item();
	void UnEquip_Item();

public:
	void Set_Player(CLIENT* _pPlayer) { m_pPlayer = _pPlayer; }

public:
	size_t Get_InvenSize() { return m_vecInven.size(); }
	INFO& Save_Info(size_t _iIdx);
	TYPE& Save_Type(size_t _iIdx);
	//STATE& Save_State(size_t _iIdx);
	void Load_Item(CObj* _pItem);

private:
	CLIENT* m_pPlayer;
	vector<CObj*>	m_vecInven;
	const size_t	m_iSize;
};


#endif // !__INVEN_H__

