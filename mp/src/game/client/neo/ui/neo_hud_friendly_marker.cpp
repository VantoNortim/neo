#include "cbase.h"
#include "neo_hud_friendly_marker.h"

#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IScheme.h>

#include <engine/ivdebugoverlay.h>
#include "ienginevgui.h"

#include "neo_gamerules.h"

#include "c_neo_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "c_team.h"
#include "tier0/memdbgon.h"

using vgui::surface;

ConVar neo_friendly_marker_hud_scale_factor("neo_friendly_marker_hud_scale_factor", "0.5", FCVAR_USERINFO,
	"Friendly player marker HUD element scaling factor", true, 0.01, false, 0);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(FriendlyMarker, 0.01)

void CNEOHud_FriendlyMarker::UpdateStateForNeoHudElementDraw()
{
}

CNEOHud_FriendlyMarker::CNEOHud_FriendlyMarker(const char* pElemName, vgui::Panel* parent)
	: CNEOHud_WorldPosMarker(pElemName, parent)
{
	SetAutoDelete(true);

	vgui::HScheme neoscheme = vgui::scheme()->LoadSchemeFromFileEx(
		enginevgui->GetPanel(PANEL_CLIENTDLL), "resource/ClientScheme_Neo.res", "ClientScheme_Neo");
	SetScheme(neoscheme);

	if (parent)
	{
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}

	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	SetBounds(0, 0, wide, tall);

	// NEO HACK (Rain): this is kind of awkward, we should get the handle on ApplySchemeSettings
	vgui::IScheme *scheme = vgui::scheme()->GetIScheme(neoscheme);
	Assert(scheme);

	m_hFont = scheme->GetFont("NHudOCRSmall", true);

	m_hTex = surface()->CreateNewTextureID();
	Assert(m_hTex > 0);
	surface()->DrawSetTextureFile(m_hTex, "vgui/hud/star", 1, false);

	surface()->DrawGetTextureSize(m_hTex, m_iMarkerTexWidth, m_iMarkerTexHeight);

	SetFgColor(Color(0, 0, 0, 0));
	SetBgColor(Color(0, 0, 0, 0));

	SetVisible(true);
}

/*void CNEOHud_FriendlyMarker::UpdateStateForNeoHudElementDraw()
{
	int x, y;
	const float scale = neo_friendly_marker_hud_scale_factor.GetFloat();
	const float heightOffset = 48.0f;
	Vector pos;

	auto localPlayer = C_NEO_Player::GetLocalNEOPlayer();

	Assert(localPlayer->m_rvFriendlyPlayerPositions.Count() == MAX_PLAYERS);

	COMPILE_TIME_ASSERT(ARRAYSIZE(m_x0) == MAX_PLAYERS);
	COMPILE_TIME_ASSERT(ARRAYSIZE(m_x1) == MAX_PLAYERS);
	COMPILE_TIME_ASSERT(ARRAYSIZE(m_y0) == MAX_PLAYERS);
	COMPILE_TIME_ASSERT(ARRAYSIZE(m_y1) == MAX_PLAYERS);

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if ((localPlayer->entindex() == i + 1) || // Skip self
			(localPlayer->m_rvFriendlyPlayerPositions[i] == vec3_origin)) // Skip unused positions
		{
			m_x0[i] = m_x1[i] = m_y0[i] = m_y1[i] = 0;
			continue;
		}

		pos = Vector(
			localPlayer->m_rvFriendlyPlayerPositions[i].x,
			localPlayer->m_rvFriendlyPlayerPositions[i].y,
			localPlayer->m_rvFriendlyPlayerPositions[i].z + heightOffset);

		if (GetVectorInScreenSpace(pos, x, y))
		{
			m_x0[i] = RoundFloatToInt((x - ((m_iMarkerTexWidth * 0.5f) * scale)));
			m_x1[i] = RoundFloatToInt((x + ((m_iMarkerTexWidth * 0.5f) * scale)));
			m_y0[i] = RoundFloatToInt((y - ((m_iMarkerTexHeight * 0.5f) * scale)));
			m_y1[i] = RoundFloatToInt((y + ((m_iMarkerTexHeight * 0.5f) * scale)));
		}
	}
}*/

void CNEOHud_FriendlyMarker::DrawNeoHudElement()
{
	if (!ShouldDraw())
	{
		return;
	}
	const float scale = neo_friendly_marker_hud_scale_factor.GetFloat();

	m_iMarkerWidth = (m_iMarkerTexWidth * 0.5f) * scale;
	m_iMarkerHeight = (m_iMarkerTexHeight * 0.5f) * scale;	

	auto localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	auto team = localPlayer->GetTeam();
	m_IsSpectator = team->GetTeamNumber() == TEAM_SPECTATOR;
	auto memberCount = team->GetNumPlayers();
	
	if(m_IsSpectator)
	{
		for(int t = 0; t < 2; ++t)
		{
			team = GetGlobalTeam(TEAM_SPECTATOR + t + 1);
			memberCount = team->GetNumPlayers();
			for (int i = 0; i < memberCount; ++i)
			{
				auto player = team->GetPlayer(i);
				if(player && player->IsAlive())
				{
					DrawPlayer(GetTeamColour(team->GetTeamNumber()), player);
				}		
			}
		}
	} else
	{
		for (int i = 0; i < memberCount; ++i)
		{
			auto player = team->GetPlayer(i);
			if(player && localPlayer->entindex() != player->entindex() && player->IsAlive())
			{
				DrawPlayer(GetTeamColour(team->GetTeamNumber()), player);
			}		
		}
	}
}

void CNEOHud_FriendlyMarker::DrawPlayer(Color teamColor, C_BasePlayer* player) const
{
	int x, y;
	constexpr float heightOffset = 48.0f;
	auto pPos = player->GetAbsOrigin();
	auto pos = Vector(
		pPos.x,
		pPos.y,
		pPos.z + heightOffset);

	if (GetVectorInScreenSpace(pos, x, y))
	{
		auto n = dynamic_cast<C_NEO_Player*>(player);
		auto a = n->m_rvFriendlyPlayerPositions;
		wchar_t playerNameUnicode[32 + 1];
		auto playerName = player->GetPlayerName();
		g_pVGuiLocalize->ConvertANSIToUnicode(player->GetPlayerName(), playerNameUnicode, sizeof(playerNameUnicode));

		auto fadeTextMultiplier = GetFadeValueTowardsScreenCentreInAndOut(x, y, 0.05);
		if(fadeTextMultiplier > 0.001)
		{
			surface()->DrawSetTextFont(m_hFont);
			surface()->DrawSetTextColor(FadeColour(teamColor, fadeTextMultiplier));
			int textWidth, textHeight;
			surface()->GetTextSize(m_hFont, playerNameUnicode, textWidth, textHeight);
			surface()->DrawSetTextPos(x - (textWidth / 2), y + m_iMarkerHeight);
			surface()->DrawPrintText(playerNameUnicode, min(sizeof(playerName),32));
		}
			
		auto fadeMarkerMultiplier = GetFadeValueTowardsScreenCentreInverted(x, y, 0.05);
		auto fadedMarkerColour = FadeColour(teamColor, fadeMarkerMultiplier);

		surface()->DrawSetTexture(m_hTex);
		surface()->DrawSetColor(fadedMarkerColour);
		surface()->DrawTexturedRect(
			x - m_iMarkerWidth,
			y - m_iMarkerHeight,
			x + m_iMarkerWidth,
			y + m_iMarkerHeight
		);
	}
}

Color CNEOHud_FriendlyMarker::GetTeamColour(int team)
{
	return (team == TEAM_NSF) ? COLOR_NSF : COLOR_JINRAI;
}

void CNEOHud_FriendlyMarker::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}
