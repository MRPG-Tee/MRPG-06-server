#ifndef GAME_SERVER_CORE_UTILITIES_COOLDOWN_H
#define GAME_SERVER_CORE_UTILITIES_COOLDOWN_H

class CGS;
class CPlayer;

class CCooldown
{
public:
	using CCooldownCallback = std::function<void()>;

	CCooldown() = default;
	void Init(int clientID) { m_ClientID = clientID; }
	void Start(int time, std::string name, CCooldownCallback callback);
	void Reset();
	bool IsCooldownActive() const { return m_IsCooldownActive; }
	void Handler();

private:
	void EndCooldown(const char* message = "\0");
	bool HasPlayerMoved(CPlayer* pPlayer) const;
	bool HasMouseMoved(CPlayer* pPlayer) const;
	void BroadcastCooldown(IServer* pServer) const;

	std::string m_Name {};
	int m_ClientID {};
	vec2 m_StartPos {};
	vec2 m_StartMousePos {};
	int m_Timer {};
	int m_StartTimer {};
	CCooldownCallback m_Callback {};
	bool m_IsCooldownActive {};
	bool m_Interrupted {};
};

#endif // GAME_SERVER_CORE_UTILITIES_COOLDOWN_H