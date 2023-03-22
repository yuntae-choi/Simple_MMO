#include "pch.h"
#include "CLIENT.h"

#include "Obj.h"
#include "Item.h"

void CLIENT::Item_Ability(int _iAtt, int _iHp)
{
	_atk += _iAtt;
	_max_hp += _iHp;
}

void CLIENT::Equip_Item(CObj* _pItem)
{
	TYPE	eType = dynamic_cast<CItem*>(_pItem)->Get_Type();

	if (m_pItem[eType])
	{
		dynamic_cast<CItem*>(m_pItem[eType])->Set_State(STATE_UNEQUIP);
		Item_Ability(-m_pItem[eType]->Get_Att(), -m_pItem[eType]->Get_Hp());
		m_pItem[eType] = nullptr;
	}

	Item_Ability(_pItem->Get_Att(), _pItem->Get_Hp());
	m_pItem[eType] = _pItem;
	dynamic_cast<CItem*>(_pItem)->Set_State(STATE_EQUIP);
}

void CLIENT::UnEquip_Item(int _iIdx)
{
	if (TYPE_END <= _iIdx)
		return;

	dynamic_cast<CItem*>(m_pItem[_iIdx])->Set_State(STATE_UNEQUIP);
	Item_Ability(-m_pItem[_iIdx]->Get_Att(), -m_pItem[_iIdx]->Get_Hp());
	m_pItem[_iIdx] = nullptr;
}