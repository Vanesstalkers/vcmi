#include "StdInc.h"
#include "CModHandler.h"
#include "JsonNode.h"
#include "Filesystem\CResourceLoader.h" //TODO: reorganize
/*
 * CModHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CArtHandler;
class CHeroHandler;
class CCreatureHandler;
class CSpellHandler;
class CBuildingHandler;
class CObjectHandler;
class CDefObjInfoHandler;
class CTownHandler;
class CGeneralTextHandler;

CModHandler::CModHandler()
{
	VLC->modh = this;

	loadConfigFromFile ("defaultMods");
	//CResourceHandler::loadModsFilesystems(); //scan for all mods
	//TODO: mod filesystem is already initialized at LibClasses launch
	//TODO: load default (last?) config
}
artID CModHandler::addNewArtifact (CArtifact * art)
{
	int id = artifacts.size();
	artifacts.push_back (art);
	return id;
}
creID CModHandler::addNewCreature (CCreature * cre)
{
	int id = creatures.size();
	creatures.push_back (cre);
	return id;
}

void CModHandler::loadConfigFromFile (std::string name)
{

	const JsonNode config(ResourceID("config/" + name + ".json"));
	auto hardcodedFeatures = config["hardcodedFeatures"];

	settings.CREEP_SIZE = hardcodedFeatures["CREEP_SIZE"].Float();
	settings.WEEKLY_GROWTH = hardcodedFeatures["WEEKLY_GROWTH_PERCENT"].Float();
	settings.NEUTRAL_STACK_EXP = hardcodedFeatures["NEUTRAL_STACK_EXP_DAILY"].Float();
	settings.MAX_BUILDING_PER_TURN = hardcodedFeatures["MAX_BUILDING_PER_TURN"].Float();
	settings.DWELLINGS_ACCUMULATE_CREATURES = hardcodedFeatures["DWELLINGS_ACCUMULATE_CREATURES"].Bool();
	settings.ALL_CREATURES_GET_DOUBLE_MONTHS = hardcodedFeatures["ALL_CREATURES_GET_DOUBLE_MONTHS"].Bool();

	auto gameModules = config["modules"];
	modules.STACK_EXP = gameModules["STACK_EXPERIENCE"].Bool();
	modules.STACK_ARTIFACT = gameModules["STACK_ARTIFACTS"].Bool();
	modules.COMMANDERS = gameModules["COMMANDERS"].Bool();
	modules.MITHRIL = gameModules["MITHRIL"].Bool();

	//auto mods = config["activeMods"]; //TODO: load only mods from the list

	auto resourceLoader = CResourceHandler::get();
	auto iterator = resourceLoader->getIterator([](const ResourceID & ident) ->  bool
	{
		std::string name = ident.getName();

		return ident.getType() == EResType::TEXT
		    && std::count(name.begin(), name.end(), '/') == 3
		    && boost::algorithm::starts_with(name, "ALL/MODS/")
		    && boost::algorithm::ends_with(name, "MOD"); //all mods have "mod.json" name - does it make sense?
	});

	std::set<std::string> foundMods;
	while (iterator.hasNext())
	{
		foundMods.insert(iterator->getName());
		++iterator;
	}

	BOOST_FOREACH (auto mod, foundMods)
	{
		tlog3 << "\t\tFound mod file: " << mod << "\n";

		const JsonNode config (ResourceID("mod"));
		const JsonNode *value = &config["creatures"];
		if (!value->isNull())
		{
			BOOST_FOREACH (auto creature, value->Vector())
			{
				auto cre = loadCreature (creature);
				addNewCreature (cre);
			}
		}
	}
}
void CModHandler::saveConfigToFile (std::string name)
{
	//JsonNode savedConf = config;
	//JsonNode schema(ResourceID("config/defaultSettings.json"));

	//savedConf.Struct().erase("session");
	//savedConf.minimize(schema);

	CResourceHandler::get()->createResource("config/" + name +".json");

	std::ofstream file(CResourceHandler::get()->getResourceName(ResourceID("config/" + name +".json")), std::ofstream::trunc);
	//file << savedConf;
}

CCreature * CModHandler::loadCreature (const JsonNode &node)
{
	CCreature * cre = new CCreature();
	cre->idNumber = creatures.size();
	const JsonNode *value; //optional value

	//TODO: ref name
	auto name = node["name"];
	cre->nameSing = name["singular"].String();
	cre->namePl = name["plural"].String();
	//TODO: map name->id

	auto cost = node["cost"];
	if (cost.getType() == JsonNode::DATA_FLOAT) //gold
	{
		cre->cost[Res::GOLD] = cost.Float();
	}
	else
	{
		int i = 0;
		BOOST_FOREACH (auto val, cost.Vector())
		{
			cre->cost[i++] = val.Float();
		}
	}

	cre->level = node["level"].Float();
	cre->faction = -1; //TODO: node["faction"].String() to id or just node["faction"].Float();
	cre->fightValue = node["fightValue"].Float();
	cre->AIValue = node["aiValue"].Float();
	cre->growth = node["growth"].Float();

	cre->addBonus(node["hitPoints"].Float(), Bonus::STACK_HEALTH);
	cre->addBonus(node["speed"].Float(), Bonus::STACKS_SPEED);
	cre->addBonus(node["attack"].Float(), Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK);
	cre->addBonus(node["defense"].Float(), Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE);
	auto vec = node["damage"].Vector();
	cre->addBonus(vec[0].Float(), Bonus::CREATURE_DAMAGE, 1);
	cre->addBonus(vec[1].Float(), Bonus::CREATURE_DAMAGE, 2);

	//optional
	value = &node["shots"];
	if (!value->isNull())
		cre->addBonus(value->Float(), Bonus::SHOTS);

	value = &node["spellPoints"];
	if (!value->isNull())
		cre->addBonus(value->Float(), Bonus::CASTS);

	cre->doubleWide = value->Bool();

	value = &node["abilities"];
	if (!value->isNull())
	{
		BOOST_FOREACH (const JsonNode &bonus, value->Vector())
		{
			cre->addNewBonus(ParseBonus(bonus));
		}
	}
	//graphics

	auto graphics = node["graphics"];
	cre->animDefName = graphics["animation"].String();
	cre->timeBetweenFidgets = graphics["timeBetweenFidgets"].Float();
	cre->troopCountLocationOffset = graphics["troopCountLocationOffset"].Float();
	cre->attackClimaxFrame = graphics["attackClimaxFrame"].Float();

	auto animationTime = graphics["animationTime"];
	cre->walkAnimationTime = animationTime["walk"].Float();
	cre->attackAnimationTime = animationTime["attack"].Float();
	cre->flightAnimationDistance = animationTime["flight"].Float(); //?
	//TODO: background?
	auto missle = graphics["missle"];
	auto offsets = missle["offset"];
	cre->upperRightMissleOffsetX = offsets["upperX"].Float();
	cre->upperRightMissleOffsetY = offsets["upperY"].Float();
	cre->rightMissleOffsetX = offsets["middleX"].Float();
	cre->rightMissleOffsetY = offsets["middleY"].Float();
	cre->lowerRightMissleOffsetX = offsets["lowerX"].Float();
	cre->lowerRightMissleOffsetY = offsets["lowerY"].Float();
	int i = 0;
	BOOST_FOREACH (auto angle, missle["frameAngles"].Vector())
	{
		cre->missleFrameAngles[i++] = angle.Float();
	}
	//we need to know creature id to add it
	VLC->creh->idToProjectile[cre->idNumber] = value->String();

	auto sounds = node["sound"];

#define GET_SOUND_VALUE(value_name) do { value = &node[#value_name]; if (!value->isNull()) cre->sounds.value_name = sounds[#value_name].String(); } while(0)
	GET_SOUND_VALUE(attack);
	GET_SOUND_VALUE(defend);
	GET_SOUND_VALUE(killed);
	GET_SOUND_VALUE(move);
	GET_SOUND_VALUE(shoot);
	GET_SOUND_VALUE(wince);
	GET_SOUND_VALUE(ext1);
	GET_SOUND_VALUE(ext2);
	GET_SOUND_VALUE(startMoving);
	GET_SOUND_VALUE(endMoving);
#undef GET_SOUND_VALUE

	creatures.push_back(cre);
	return cre;
}
void CModHandler::recreateHandlers()
{
	//TODO: consider some template magic to unify all handlers?

	//VLC->arth->artifacts.clear();
	//VLC->creh->creatures.clear(); //TODO: what about items from original game?

	BOOST_FOREACH (auto creature, creatures)
	{
		VLC->creh->creatures.push_back (creature);
	}

	BOOST_FOREACH (auto mod, activeMods) //inactive part
	{
		BOOST_FOREACH (auto art, allMods[mod].artifacts)
		{
			VLC->arth->artifacts.push_back (artifacts[art]);

			//TODO: recreate types / limiters based on string id
		}
		BOOST_FOREACH (auto creature, allMods[mod].creatures)
		{
			VLC->creh->creatures.push_back (creatures[creature]);
			//TODO VLC->creh->notUsedMonster.push_back (creatures[creature]);

			//TODO: recreate upgrades and other properties based on string id
		}
	}
}

CModHandler::~CModHandler()
{
}