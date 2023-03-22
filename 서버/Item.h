#pragma once
#include "pch.h"
#include "CLIENT.h"

#ifndef __ITEM_H__
#define __ITEM_H__

#include "Obj.h"
class CItem : public CObj
{
public:
	CItem();
	CItem(INFO& _rInfo);
	virtual ~CItem();

public:
	virtual void Initialize() override;
	virtual void Render() override;
	virtual void Release() override;

public:
	TYPE& Get_Type() { return m_eType; }
	ITEM_STATE& Get_State() { return m_eState; }

public:
	void Set_Type(TYPE _eType) { m_eType = _eType; }
	void Set_State(ITEM_STATE _eState) { m_eState = _eState; }

public:
	INFO& Save_Data() { return m_tInfo; }

private:
	TYPE	m_eType;
	ITEM_STATE	m_eState;
};


#endif // !__ITEM_H__
