#include "Player.h"

USING_NS_CC;

#include "Monster.h"
#include "LogicManager.h"
#include "CSVFileManager.h"
#include "ResManager.h"
#include "SceneManager.h"
#include "BaseFunction.h"
#include "AStar.h"
#include "ArmatureExtend.h"
#include "NetManager.h"
#include "CCActionInstant.h"
#include "deprecated/CCString.h"
#include "MoveComponent.h"
#include "UIManager.h"
#include "UIMain.h"

static const int DefaultBodyFace = 1;
static const int DefaultHorse = 1;
static const int DefaultWing = 1;

Player::Player()
{
    m_IsRide = false;
    m_HeadIconIndex = 0;
    m_Sex = 0;
    m_Level = 1;

    m_Hp = 0;                   
    m_HpMax = 0;

    m_IsSkilling = false;
}

Player::~Player()
{
    delete m_AnimCompoenet;
    delete m_MoveCompoenet;
    delete m_BattleCompoent;
}

bool Player::Init(uint64 guid,int sex, const char* name, int face[EQUIPTYPE_MAX], bool dead)
{
    m_GUID = guid;
    m_Type = OBJECT_PLAYER;
    m_Sex = sex;
    m_Name = name;

	m_HeadIconIndex = 1;
	m_IsRide = 0;
	
    m_AnimCompoenet = new PlayerAnimComp(this, this);
    m_MoveCompoenet = new MoveComponent(this, this);
    m_BattleCompoent = new BattleComponent(this, this);

	m_HeadIcon = "Icon/HeroIcon_1.png";
	m_Type = OBJECT_PLAYER;

	ReloadFace(face);
    
    if (!dead)
        Idle();
    else
        InitDie();

    m_AnimCompoenet->EnableShadow(true);
    m_AnimCompoenet->EnableHpBar(true);
    m_AnimCompoenet->SetHpBarPercent(100.0f);

	const NameColor *nameColor;
	//是否是玩家自己
	if (g_LogicManager->m_Hero->m_GUID == m_GUID)
	{
		nameColor = g_CSVFileManager->GetNameColorByID(1);
	}
	else
	{
		//是否是队友
		if (g_LogicManager->GetPlayerIsTeam(m_GUID))	
		{
			nameColor = g_CSVFileManager->GetNameColorByID(2);
		}
		else
		{
			//是否是同一阵营
			if (g_LogicManager->GetPlayerIsCamp(m_GUID))	
			{
				nameColor = g_CSVFileManager->GetNameColorByID(3);
			}
			else
			{
				nameColor = g_CSVFileManager->GetNameColorByID(4);
			}
		}
	}

    SetName(m_Name, 185.0f, Color3B(nameColor->iR, nameColor->iG, nameColor->iB));
    
    schedule(CC_SCHEDULE_SELECTOR(Player::Update), 0.1f);
	return true;
}

void Player::ReleaseSelf(Node *sender, long data)
{
	removeFromParentAndCleanup(true);
}

void Player::Update(float delta)
{
	if (m_AnimCompoenet)
	{
        m_AnimCompoenet->Update(delta);
	}

    if (m_BattleCompoent)
    {
        m_BattleCompoent->Update(delta);
    }

    int oldZOrder = getLocalZOrder();
    const Vec2& position = getPosition();
    int newZOrder = 10000 - (int)position.y;
    if (oldZOrder != newZOrder)
    {
        ApplyMapTileAttribute();
    }
}

void Player::Idle()
{
    MapObj::Idle();

	if (m_IsRide)
	{
		m_AnimCompoenet->ChangeAction(ACTION_HORSE_IDLE);
	}
	else
	{
		m_AnimCompoenet->ChangeAction(ACTION_IDLE);
	}
}

void Player::Die()
{
    MapObj::Die();
    if (m_AnimCompoenet)
    {
        m_AnimCompoenet->ChangeAction(ACTION_DEAD);
    }
    g_ResManager->PlaySound(104);
}

void Player::Ride(bool ride)
{
    /*
	if (m_IsRide != ride)
	{
		m_IsRide = ride;

		if (m_IsRide)
		{
            if(m_Horse){
			    m_Horse->setVisible(true);
            }
			if (m_ActionType == ACTION_IDLE)
			{
				m_ActionType = ACTION_HORSE_IDLE;
			}
			else if (m_ActionType == ACTION_MOVE)
			{
				m_ActionType = ACTION_HORSE_MOVE;
			}
		}
		else
		{
            if(m_Horse){
			    m_Horse->setVisible(false);
            }
			if (m_ActionType == ACTION_HORSE_IDLE)
			{
				m_ActionType = ACTION_IDLE;
			}
			else if (m_ActionType == ACTION_HORSE_MOVE)
			{
				m_ActionType = ACTION_MOVE;
			}
		}

		UpdataAction();
	}
    */
}

Node* Player::CreateFaceCopyNode()
{
    return CreateFaceCopyNode(m_Face, m_Sex);
}

Node* Player::CreateFaceCopyNode(int equipId[EQUIPTYPE_MAX], int sex)
{
    Node* copyNode = Node::create();
    static char path[64];
    static string name;

    //>同步创建,可能会卡.
    if(1)
    {
        Sprite* pBody = Sprite::create();
        copyNode->addChild(pBody);
        int faceId = equipId[EQUIPTYPE_BODY] != 0 ? equipId[EQUIPTYPE_BODY] : DefaultBodyFace;
        faceId = equipId[EQUIPTYPE_FASHION] != 0 ? equipId[EQUIPTYPE_FASHION] : faceId;
        sprintf(path, "player_%d_%d_%d_%d_%d", sex, EQUIPTYPE_BODY, faceId, ACTION_IDLE, DIRECTION_DOWN);
        name = path;
        Vector<SpriteFrame*> SpriteFrame = g_ResManager->GetSpriteFrame(name);
        if (SpriteFrame.empty()){
            g_ResManager->CreatePlayerPart(StringUtils::format("player_%d_%d_%d", sex, EQUIPTYPE_BODY, faceId)); 
            SpriteFrame = g_ResManager->GetSpriteFrame(name);
        }
        Animation* animation = Animation::createWithSpriteFrames(SpriteFrame, PLAYER_ANIMATION_FRAME_TIME);
        pBody->runAction(RepeatForever::create( Animate::create(animation)));
        int zorder = g_CSVFileManager->m_PartLayer[DIRECTION_DOWN][EQUIPTYPE_BODY];
        pBody->setLocalZOrder(zorder);
    }

    if (equipId[EQUIPTYPE_WEAPON])
    {
        Sprite* pWeapon = Sprite::create();
        copyNode->addChild(pWeapon);
        //男女共用武器~统一为男性资源
        sprintf(path, "player_%d_%d_%d_%d_%d", 1, EQUIPTYPE_WEAPON, equipId[EQUIPTYPE_WEAPON], ACTION_IDLE, DIRECTION_DOWN);
        name = path;
        Vector<SpriteFrame*> SpriteFrame = g_ResManager->GetSpriteFrame(name);
        if (SpriteFrame.empty()){
            g_ResManager->CreatePlayerPart(StringUtils::format("player_%d_%d_%d", 1, EQUIPTYPE_WEAPON, equipId[EQUIPTYPE_WEAPON]));
            SpriteFrame = g_ResManager->GetSpriteFrame(name);
        }
        Animation* animation = Animation::createWithSpriteFrames(SpriteFrame, PLAYER_ANIMATION_FRAME_TIME);
        pWeapon->runAction(RepeatForever::create( Animate::create(animation)));
        int zorder = g_CSVFileManager->m_PartLayer[DIRECTION_DOWN][EQUIPTYPE_WEAPON];
        pWeapon->setLocalZOrder(zorder);
    }

    if (1)
    {
        Sprite* pWing = Sprite::create();
        copyNode->addChild(pWing);
        //男女共用翅膀~统一为男性资源
        sprintf(path, "player_%d_%d_%d_%d_%d", 1, EQUIPTYPE_WING, equipId[EQUIPTYPE_WING] != 0 ? equipId[EQUIPTYPE_WING] : DefaultWing, ACTION_IDLE, DIRECTION_DOWN);
        name = path;
        Vector<SpriteFrame*> SpriteFrame = g_ResManager->GetSpriteFrame(name);
        if (SpriteFrame.empty()){
            g_ResManager->CreatePlayerPart(StringUtils::format("player_%d_%d_%d", 1, EQUIPTYPE_WING, equipId[EQUIPTYPE_WING]));
            SpriteFrame = g_ResManager->GetSpriteFrame(name);
        }
        Animation* animation = Animation::createWithSpriteFrames(SpriteFrame, PLAYER_ANIMATION_FRAME_TIME);
        pWing->runAction(RepeatForever::create( Animate::create(animation)));
        int zorder = g_CSVFileManager->m_PartLayer[DIRECTION_DOWN][EQUIPTYPE_WING];
        pWing->setLocalZOrder(zorder);
    }

    CCASSERT(copyNode->getChildrenCount() > 0, "");
    return copyNode;
}

void Player::EquipIdToFaceId(int equipId[EQUIPTYPE_MAX], int faceId[EQUIPTYPE_MAX])
{
    for (int i = 0; i < EQUIPTYPE_MAX; ++i)
    {
        faceId[i] = equipId[i];
        if (equipId[i] != 0)
        {
            const ItemsInfo* pItemInfo = g_CSVFileManager->GetItemsByID(equipId[i]);
            assert(pItemInfo);
            if (pItemInfo)
            {
                faceId[i] = pItemInfo->iFaceID;
            }
        }
    }
}

bool Player::ReloadFace(int face[EQUIPTYPE_MAX], bool asyncLoad/* = true*/)
{
    for (int i = 0; i < EQUIPTYPE_MAX; ++i)
    {
        if (face)
        {
            m_Face[i] = face[i];
        }else
        {
            m_Face[i] = 0;
        }
    }

    if (m_AnimCompoenet)
    {
        m_AnimCompoenet->LoadResouce(true);
    }

    return true;
}

float Player::AC_GetMoveSpeed()
{
    return 1.0f;
}

void Player::GetFaces(int face[EQUIPTYPE_MAX])
{
    for (int i = 0; i < EQUIPTYPE_MAX; ++i)
    {
        face[i] = m_Face[i];
    }
}

int Player::GetSex()
{
    return m_Sex;
}

void Player::OnAttcakAnimEnd()
{
    if (m_AnimCompoenet)
    {
        if (!IsDead())
        {
            m_AnimCompoenet->ChangeAction(ACTION_IDLE);
        }
    }
}

float Player::MC_GetMoveSpeed()
{
    return 1.0f;
}

void Player::OnChangeDirection(DirectionType direction)
{
    if (m_AnimCompoenet)
    {
        m_AnimCompoenet->ChangeDirection(direction);
    }
}

void Player::OnMoveByTarget(const cc2d::Vec2 &tagPos)
{
    if (m_AnimCompoenet)
    {
        m_AnimCompoenet->ChangeAction(ACTION_MOVE);
    }
}

void Player::OnMoveByPath(std::vector<cc2d::Vec2>& paths, const cc2d::Vec2 &tagPos)
{
    if (m_AnimCompoenet)
    {
        m_AnimCompoenet->ChangeAction(ACTION_MOVE);
    }
}

void Player::OnMoveEnd()
{
    if (m_AnimCompoenet)
    {
        m_AnimCompoenet->ChangeAction(ACTION_IDLE);
    }
}

inline void Player::SetHp(int hp)
{
    m_Hp = hp;    
    if (m_AnimCompoenet)
    {
        float percent = m_Hp * 100.0f / m_HpMax;
        m_AnimCompoenet->SetHpBarPercent(percent);
    }

    if (m_Hp <= 0 && !IsDead())
        Die();
}
inline void Player::SetHpMax(int hpMax)
{
    m_HpMax = hpMax;
    if (m_AnimCompoenet)
    {
        float percent = m_Hp * 100.0f / m_HpMax;
        m_AnimCompoenet->SetHpBarPercent(percent);
    }
}

void Player::InitDie()
{
    MapObj::Die();
    if (m_AnimCompoenet)
    {
        static_cast<PlayerAnimComp*>(m_AnimCompoenet)->ResetDieActionToLastFrame();
        m_AnimCompoenet->ChangeAction(ACTION_DEAD);
    }
}

void Player::OnSkillBegin(int skillId, MapObj* target)
{
    m_IsSkilling = true;
}

void Player::OnSkillEnd()
{
    m_IsSkilling = false;
}

void Player::OnInitBuffList()
{
    if (m_AnimCompoenet)
    {
        auto& buffDataList = m_BattleCompoent->GetBuffDatas();
        auto it = buffDataList.begin();
        for (auto end = buffDataList.end(); it != end; ++it)
        {
            auto& buffData = *it;
            if (buffData.pBuffInfo->iTargetEffect > 0)
            {
                bool isLoop = false;
                cc2d::Sprite* pEffect = g_ResManager->CreateBuffSprite(buffData.pBuffInfo->iTargetEffect, isLoop);
                if (pEffect)
                {
                    m_AnimCompoenet->PlayEffect(pEffect);
                    if (isLoop)
                    {
                        buffData.pUserData = pEffect;
                    }
                }
            }
        }
    }

    //>刷新界面??
    if (g_LogicManager->m_Hero->m_SelTarget && g_LogicManager->m_Hero->m_SelTarget->m_GUID == m_GUID)
    {
        UIMain* pUIMain = (UIMain*)g_UIManager->GetUI(UI_MAIN);
        if (pUIMain)
        {
            pUIMain->LoadTargetBuffIcon();
        }
    }

    if (m_GUID == g_LogicManager->m_Hero->m_GUID)
    {
        UIMain* pUIMain = (UIMain*)g_UIManager->GetUI(UI_MAIN);
        if (pUIMain)
        {
            pUIMain->LoadHeroBuffIcon();
        }
    }
}

void Player::OnAddBuff(const BattleComponent::BuffData& buffData)
{
    if (m_AnimCompoenet)
    {
        if (buffData.pBuffInfo->iTargetEffect > 0)
        {
            bool isLoop = false;
            cc2d::Sprite* pEffect = g_ResManager->CreateBuffSprite(buffData.pBuffInfo->iTargetEffect, isLoop);
            if (pEffect)
            {
                m_AnimCompoenet->PlayEffect(pEffect);
                if (isLoop)
                {
                    buffData.pUserData = pEffect;
                }
            }
        }
    }

    //>刷新界面??
    if (g_LogicManager->m_Hero->m_SelTarget && g_LogicManager->m_Hero->m_SelTarget->m_GUID == m_GUID)
    {
        UIMain* pUIMain = (UIMain*)g_UIManager->GetUI(UI_MAIN);
        if (pUIMain)
        {
            pUIMain->LoadTargetBuffIcon();
        }
    }

    if (m_GUID == g_LogicManager->m_Hero->m_GUID)
    {
        UIMain* pUIMain = (UIMain*)g_UIManager->GetUI(UI_MAIN);
        if (pUIMain)
        {
            pUIMain->LoadHeroBuffIcon();
        }
    }
}

void Player::OnRemoveBuff(const BattleComponent::BuffData& buffData)
{
    if (m_AnimCompoenet)
    {
        if (buffData.pUserData != NULL)
        {
            cc2d::Sprite* pEffect = (cc2d::Sprite*)buffData.pUserData;
            pEffect->stopAllActions();
            pEffect->removeFromParent();
        }
    }

    //>刷新界面??
    if (g_LogicManager->m_Hero->m_SelTarget && g_LogicManager->m_Hero->m_SelTarget->m_GUID == m_GUID)
    {
        UIMain* pUIMain = (UIMain*)g_UIManager->GetUI(UI_MAIN);
        if (pUIMain)
        {
            pUIMain->LoadTargetBuffIcon();
        }
    }

    if (m_GUID == g_LogicManager->m_Hero->m_GUID)
    {
        UIMain* pUIMain = (UIMain*)g_UIManager->GetUI(UI_MAIN);
        if (pUIMain)
        {
            pUIMain->LoadHeroBuffIcon();
        }
    }
}


