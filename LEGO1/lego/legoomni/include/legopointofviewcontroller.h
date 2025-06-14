#ifndef LEGOPOINTOFVIEWCONTROLLER_H
#define LEGOPOINTOFVIEWCONTROLLER_H

#include "decomp.h"
#include "mxcore.h"
#include "mxgeometry.h"

#ifdef MINIWIN
#include "miniwin/windows.h"
#else
#include "miniwin/windows.h"
#endif

class Lego3DView;
class LegoEntity;
class LegoNavController;

//////////////////////////////////////////////////////////////////////////////
//
// LegoMouseController

// VTABLE: LEGO1 0x100d8dd8
// SIZE 0x20
class LegoMouseController : public MxCore {
public:
	LegoMouseController();
	~LegoMouseController() override;

	virtual void LeftDown(int, int);  // vtable+0x14
	virtual void LeftDrag(int, int);  // vtable+0x18
	virtual void LeftUp(int, int);    // vtable+0x1c
	virtual void RightDown(int, int); // vtable+0x20
	virtual void RightDrag(int, int); // vtable+0x24
	virtual void RightUp(int, int);   // vtable+0x28

	BOOL GetIsButtonDown() { return m_isButtonDown; }
	MxDouble GetButtonX() { return m_buttonX; }
	MxDouble GetButtonY() { return m_buttonY; }

private:
	BOOL m_isButtonDown;  // 0x08
	undefined4 m_unk0x0c; // 0x0c
	MxDouble m_buttonX;   // 0x10
	MxDouble m_buttonY;   // 0x18
};

// SYNTHETIC: LEGO1 0x100655b0
// LegoMouseController::`scalar deleting destructor'

// VTABLE: LEGO1 0x100d8e08
// SIZE 0x38
class LegoPointOfViewController : public LegoMouseController {
public:
	LegoPointOfViewController();
	~LegoPointOfViewController() override;

	MxResult Tickle() override;               // vtable+0x08
	void LeftDown(int p_x, int p_y) override; // vtable+0x0c
	void LeftDrag(int p_x, int p_y) override; // vtable+0x10

	// FUNCTION: LEGO1 0x10011e40
	void LeftUp(int p_x, int p_y) override
	{
		LegoMouseController::LeftUp(p_x, p_y);
		AffectPointOfView();
	}
	// vtable+0x14

	// FUNCTION: LEGO1 0x10011e60
	void RightDown(int p_x, int p_y) override
	{
		LegoMouseController::RightDown(p_x, p_y);
		AffectPointOfView();
	}
	// vtable+0x20

	// FUNCTION: LEGO1 0x10011e80
	void RightDrag(int p_x, int p_y) override
	{
		LegoMouseController::RightDrag(p_x, p_y);
		AffectPointOfView();
	}
	// vtable+0x24

	// FUNCTION: LEGO1 0x10011ea0
	void RightUp(int p_x, int p_y) override
	{
		LegoMouseController::RightUp(p_x, p_y);
		AffectPointOfView();
	}                                             // vtable+0x28
	virtual void SetEntity(LegoEntity* p_entity); // vtable+0x2c

	MxResult Create(Lego3DView* p_lego3DView);
	void OnViewSize(int p_width, int p_height);

	LegoEntity* GetEntity() { return m_entity; }
	LegoNavController* GetNavController() { return m_nav; }

protected:
	void AffectPointOfView();

	Lego3DView* m_lego3DView;  // 0x20
	LegoEntity* m_entity;      // 0x24
	MxDouble m_entityOffsetUp; // 0x28
	LegoNavController* m_nav;  // 0x30
};

// SYNTHETIC: LEGO1 0x10065750
// LegoPointOfViewController::`scalar deleting destructor'

#endif /* LEGOPOINTOFVIEWCONTROLLER_H */
