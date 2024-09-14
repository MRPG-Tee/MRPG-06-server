/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>

enum
{
	CANTMOVE_LEFT = 1 << 0,
	CANTMOVE_RIGHT = 1 << 1,
	CANTMOVE_UP = 1 << 2,
	CANTMOVE_DOWN = 1 << 3,
};

class CTile;
class CTeleTile;
class CSwitchTileExtra;
class CLayers;

class CCollision
{
public:
	enum
	{
		COLFLAG_SOLID = 1,
		COLFLAG_DEATH = 2,
		COLFLAG_NOHOOK = 4,
		COLFLAG_SAFE = 1 << 3,
		COLFLAG_DISALLOW_MOVE = 1 << 4,
	};

private:
	int m_Width{};
	int m_Height{};
	CTile *m_pTiles{};
	CTile *m_pFront{};
	CTeleTile* m_pTele{};
	CSwitchTileExtra* m_pExtra{};
	CLayers* m_pLayers {};

	// elements
	struct FixedCamZoneData
	{
		vec2 Pos;
		vec4 Rect;
		bool Smooth;
	};
	std::map<int, std::vector<vec2>> m_vTeleOuts {};
	std::vector<FixedCamZoneData> m_vFixedCamZones {};
	std::map<std::string, vec2> m_vInteractObjects {};
	std::map<int, std::string> m_vZoneNames {};

	// initialization
	void InitTiles(CTile* pTiles);
	void InitTeleports();
	void InitExtra();

	// flags
	int GetMainTileFlags(float x, float y) const;
	int GetFrontTileFlags(float x, float y) const;
	int GetExtraTileFlags(float x, float y) const;

	// collision flags
	int GetMainTileCollisionFlags(int x, int y) const;
	int GetFrontTileCollisionFlags(int x, int y) const;

public:
	CCollision();
	~CCollision();

	void Init(class IKernel* pKernel, int WorldID);
	void InitEntities(const std::function<void(int, vec2, int)>& funcInit) const;

	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }
	vec2 GetRotateDirByFlags(int Flags);
	CLayers* GetLayers() const { return m_pLayers; }

	// tile index
	int GetMainTileIndex(float x, float y) const;
	int GetFrontTileIndex(float x, float y) const;
	int GetExtraTileIndex(float x, float y) const;

	// collision flags
	bool CheckPoint(float x, float y, int Flag = COLFLAG_SOLID) const { return (GetCollisionFlagsAt(x, y) & Flag) != 0; }
	int GetCollisionFlagsAt(float x, float y) const
	{
		const ivec2 roundPos(round_to_int(x), round_to_int(y));
		const int TileCollisionFlags = GetMainTileCollisionFlags(roundPos.x, roundPos.y);
		const int FrontTileCollisionFlags = GetFrontTileCollisionFlags(roundPos.x, roundPos.y);
		return TileCollisionFlags | FrontTileCollisionFlags;
	}
	bool CheckPoint(vec2 Pos, int Flag = COLFLAG_SOLID) const { return CheckPoint(Pos.x, Pos.y, Flag); }
	int GetCollisionFlagsAt(vec2 Pos) const { return GetCollisionFlagsAt(Pos.x, Pos.y); }

	// flags
	int GetFlagsAt(float x, float y) const
	{
		const int TileFlags = GetMainTileFlags(x, y);
		const int FrontTileFlags = GetFrontTileFlags(x, y);
		return TileFlags | FrontTileFlags;
	}
	int GetFlagsAt(vec2 Pos) const { return GetFlagsAt(Pos.x, Pos.y); }

	// self
	const char* GetZonename(vec2 Pos) const;
	std::optional<vec2> TryGetTeleportOut(vec2 currentPos);
	std::optional<std::pair<vec2, bool>> TryGetFixedCamPos(vec2 currentPos) const;

	// other
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2* pOutCollision, vec2* pOutBeforeCollision, int ColFlag = COLFLAG_SOLID) const;
	bool IntersectLineWithInvisible(vec2 Pos0, vec2 Pos1, vec2* pOutCollision, vec2* pOutBeforeCollision) const
	{
		return IntersectLineColFlag(Pos0, Pos1, pOutCollision, pOutBeforeCollision, COLFLAG_DISALLOW_MOVE | COLFLAG_SOLID);
	}
	bool IntersectLineColFlag(vec2 Pos0, vec2 Pos1, vec2* pOutCollision, vec2* pOutBeforeCollision, int ColFlag) const;
	void FillLengthWall(int DepthTiles, vec2 Direction, vec2* pPos, vec2* pPosTo, bool OffsetStartlineOneTile = true) const;
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces) const;
	void MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity, bool *pDeath=NULL) const;
	bool TestBox(vec2 Pos, vec2 Size, int Flag=COLFLAG_SOLID) const;
	void MovePhysicalAngleBox(vec2* pPos, vec2* pVel, vec2 Size, float* pAngle, float* pAngleForce, float Elasticity, float Gravity = 0.5f) const;
	void MovePhysicalBox(vec2* pPos, vec2* pVel, vec2 Size, float Elasticity, float Gravity = 0.5f) const;
	vec2 GetDoorNormal(vec2 doorStart, vec2 doorEnd, vec2 from);
};

#endif
