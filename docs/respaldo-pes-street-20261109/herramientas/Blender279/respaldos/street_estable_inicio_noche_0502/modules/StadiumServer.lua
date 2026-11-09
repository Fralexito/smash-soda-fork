-- Stadium server v1.60 for PES 2020/2021 - assign the stadium for either home team or competition
-- Custom content is used, not LiveCPK/game: content\stadium-server is the root
-- author: zlac, 2018-2022
-- stadium searching and other extra credits: juce
-- originally posted on evo-web

-- local new_stadium_path
local version = "1.60"
local stadiumroot = ".\\content\\stadium-server\\"
local stadium_switched = false
local stadium_name_switched = false
local last_choice
local last_choice_teams
local last_choice_comp
local last_choice_comp_finals
local settings

-- "minus 1" will be applied to all values > 0
local weather_TimeOfDay = 0 -- 0 (in-game selection), 1 (Day), 2 (Night)
local weather_TimeOfDay_Info = {["0"]="Por defecto", ["1"]="Día", ["2"]="Noche"}
local weather_Weather = 0 -- 0 (in-game selection), 1 (Fine/Sunny), 2 (Rainy), 3 (Snowy)
local weather_Weather_Info = {["0"]="Por defecto", ["1"]="Soleado", ["2"]="Lluvioso", ["3"]="Nevado"}
local weather_WeatherEffects = 0 -- 0 (in-game selection), 3 (Force Rain/snow falling)
local weather_WeatherEffects_Info = {["0"]="Por defecto", ["3"]="Forzado"}
local weather_Season = 0 -- 0 (in-game selection), 1 (Summer), 2 (Winter)
local weather_Season_Info = {["0"]="Por defecto", ["1"]="Verano", ["2"]="Invierno"}

--[[
local replay_stad_id = nil
local replay_source_stadium_map = nil
local replay_source_stadium_key = nil
]]--

local random_num_comp -- which stadium to select randomly in competition modes from stadium server (depending on entries available per competition in map_competitions.txt)
local random_num_team -- which stadium to select randomly if a team has more than one stadium assigned in map_teams.txt
local random_num_exhib -- which stadium to select randomly in exhibition modes from stadium server
local rnd_server_or_cpk -- to serve random stadium from stadium server (depending on entries available per competition in map_competition.txt) or from default .cpk files (i.e. stadium server deactivates itself temporarily)

local team_assignment_map = {}
local competition_assignment_map = {}
local all_stadiums_map = {}

local info_text = ""
local weather_text = ""
local manual_stad_idx
local manual_stad_info = "Estadio seleccionado manualmente: Ninguno"
local manual_selection_status = "Off"
local manual_selection_status_info = "Modo de selección: asignación automática"
local total_stadiums

local _stadium_info = {}
local current_stadium_preview_path

local RELOAD_MAPS_KEY = 0x30 -- 0 key (not the NUMPAD zero!!)
local WEATHER_TIMEOFDAY_KEY = 0x33 -- 3 key (not the NUMPAD 7!!)
local WEATHER_WEATHER_KEY = 0x34 -- 4 key (not the NUMPAD 7!!)
local WEATHER_WEATHER_EFFECTS_KEY = 0x35 -- 5 key (not the NUMPAD 7!!)
local WEATHER_SEASON_KEY = 0x36 -- 6 key (not the NUMPAD 7!!)
local SET_AS_FAVORITE_STAD_KEY = 0x37 -- 7 key (not the NUMPAD 7!!)
local USE_FAVORITE_STAD_KEY = 0x38 -- 8 key (not the NUMPAD 8!!)
local SWITCH_SELECTION_MODE_KEY = 0x39 -- 9 key (not the NUMPAD 9!!)
local DEL_TEXT_KEY = 0x2E
local PREVIOUS_STAD_KEY = 0x21 -- Page Up
local NEXT_STAD_KEY = 0x22 -- Page Down

local SEARCH_START_KEY = 0x72 -- F3
local SEARCH_END_KEY = 0x0d -- Enter
local SEARCH_BACKSPACE_KEY = 0x08 -- Backspace
local SEARCH_CANCEL_KEY = 0x1b -- Escape
local _searching
local _search_string

local stadium_config = {}

-- override_competitions contains comma-separated ID's of the competitions that allow for team-assigned stadiums to have precedence before competiton-assigned stadiums
-- .. e.g. you've assigned a home stadium for ALL exhibition mode matches (tid 65535) in map_competitions.txt, but you still want to use
-- .. custom home-team stadium for those teams that have it assigned in map_teams.txt -> add 65535 in override_competitions
-- .. OR: you have assigned multiple stadiums to be selected randomly (e.g. for EPL) for those teams that do not have their own stadiums, but you 
-- .. still want to use custom home-team stadium for those teams that have it assigned in map_teams.txt -> add 17 too (EPL id) in override_competitions
-- initially, all (at least, i hope so) Exhibition mode, League and League-Cup matches are included in competition overrides
-- BEGIN CUSTOMIZABLE LUA TABLE                         
local override_competitions = {65535,  -- Exhibition matches
								4, -- UCL knockout stage
								6, -- UEL knockout stage
								10, -- Copa Libertadores knockout stage
								16, -- ACL knockout stage 
								17, 79, 83, -- England 1st and 2nd div and cup and play offs
								19, 80, 25, 87, 84, -- Spain 1st and 2nd div and cup and supercup (2-legged supercup, home&away matches) and playoffs
								18, 82, 24, 85, -- Italy 1st and 2nd div and cup and playoffs
								20, 81, 26, -- France 1st and 2nd div and cup
								22, 28, -- Portugal league and cup
								21, 27, -- Netherlands league and cup
								30, 59, -- Brazil league and cup
								29, 163, -- Argentina 1st and 2nd div and cup
								67, 13400, 13401, 68, -- Chile league (first and second stage) and cup
								50, 53, -- PEU league and cup
								51, 10200, 10201, 54, -- PLA league (first and second stage) and cup
								52, 55, -- PAS league and cup
                                120, 127, -- China league and cup
                                115, 155, 156, 157, 158, 159, 122, -- Belgium 1st div (playoff groups 0 to 3, final knockout) and cup
                                116, 123, -- Russia 1st div and cup
                                117, 124, -- Switzerland 1st div and cup
                                118, 125, -- Turkey 1st div and cup
                                119, 23800, 23801, 160, 161, 126, -- Colombia 1st div (first and second stage, knockout 0 and 1) and cup
                                133, 134, 135, 136, 137, -- Scotland 1st div (playoff groups 0 to 2) and cup
                                141, 147, 148, 149, 150, 151, 142, -- Denmark 1st div (playoff groups 0 to 3, final knockout) and cup
                                162, 164, -- Thailand 1st div and cup
								}
-- END CUSTOMIZABLE LUA TABLE

local function nil2str(value)
    if value ~= nil then
        return value
    else
        return "N/A"
    end
end

-- log only if detailed logging is required in config.ini
local function _log(msg)
	-- are settings available & configured?
	if settings and settings["detailed_logging"] then
		if settings["detailed_logging"] == 1 then
			log(msg)
		end
	-- fallback to default logging, if not
	else
		log(msg)
	end
end

local function load_ini(filename)
    local t = {}
    local data = assert(io.lines(stadiumroot .. filename))
    _log(filename .. " found in " .. stadiumroot)

    for line in data do
        local name, value = string.match(line, "^([%w_]+)%s*=%s*([-%w%d.]+)")
        if name and value then
            value = tonumber(value) or value
            t[name] = value
            _log(string.format("Using setting: %s = %s", name, value))
        end
    end
    return t
end

local function load_addon_ini(filename)
    local t = {}
    local data = assert(io.lines(stadiumroot .. filename))
    _log(filename .. " found in " .. stadiumroot)

    for line in data do
        local name, value = string.match(line, "^([%w_]+)%s*=%s*([-%w%d_ .]+)")
        if name and value then
            value = tonumber(value) or value
            t[name] = value
            _log(string.format("Using add-on mod: %s = %s", name, value))
        end
    end
    return t
end

local function load_force_weather_ini(filename)
    local t = {}
    local data = assert(io.lines(stadiumroot .. filename))
    _log(filename .. " found in " .. stadiumroot)

    for line in data do
        local name, value = string.match(line, "^%s*([%w_]+)%s*=%s*([%w]+)")
        if name and value then
            value = tonumber(value) or value
            t[name] = value
            _log(string.format("Using forced weather settings: %s = %s", name, value))
        end
    end
    return t
end

local function save_ini(filename)
    local f = io.open(stadiumroot .. filename, "wt")
    f:write(string.format("# StadiumServer settings. Generated by StadiumServer.lua\n"))
	f:write(string.format("# detailed_logging: 1 = more extensive logging; 0 = bare-bones logging\n"))
    f:write("\n")
    local keys = {}
    for name,value in pairs(settings) do
        keys[#keys + 1] = name
    end
    table.sort(keys)
    for i,name in ipairs(keys) do
        local value = settings[name]
        f:write(string.format("%s = %s\n", name, value))
    end
    f:write("\n")
    f:close()
end


local function file_exists(name)
	local f=io.open(name,"r")
	if f~=nil then 
		io.close(f) 
		return true 
	else 
		return false 
	end
end

-- remove trailing and leading whitespace from string
local function trim(s)
    return s:gsub("^%s*(.-)%s*$", "%1")
end


local function split(s, inSplitPattern)
	local outResults = {}
	-- chop off the trailing comment, if present
	local theCommentStart = string.find( s, "#", 1 )
	local data = s
	if theCommentStart ~= nil then
		data = string.sub(s, 1, theCommentStart-1)
	end

	-- now do the splits by main separator (inSplitPattern)
	local theStart = 1
	local theSplitStart, theSplitEnd = string.find( data, inSplitPattern, theStart )
	while theSplitStart do
		outResults[#outResults+1] = trim(string.sub( data, theStart, theSplitStart-1 ))
		theStart = theSplitEnd + 1
		theSplitStart, theSplitEnd = string.find( data, inSplitPattern, theStart )
	end
	outResults[#outResults+1] = trim(string.sub( data, theStart ))
	return outResults
end

local function clear_table(t)
    for k,v in pairs(t) do
        t[k]=nil
    end
end

local function dump_table(o)
	if type(o) == 'table' then
		local s = '{ '
		for k,v in pairs(o) do
			if type(k) ~= 'number' then k = '"'..k..'"' end
			s = s .. '['..k..'] = ' .. dump_table(v) .. ','
		end
		return s .. '} '
	else
		return tostring(o)
	end
end

local function print_arr(tbl)
    for index, value in ipairs(tbl) do
        log("     " .. value)
    end
end

local function tableLength(T)
    local count = 0
    for _ in pairs(T) do 
        count = count + 1
    end
    return count
end

local function valueArrayExistsInTable(tbl, val, startAt)
    for key, value in pairs(tbl) do
        if val[startAt] == value[1] and val[startAt+1] == value[2] then
            return true
        end
    end
    return false
end

local function valueArrayExistsInTable_ForTeamsMap(tbl, val)
    for key, value in pairs(tbl) do
        if val[1] == value[1] and val[2] == value[2] and val[3] == value[3] then
            return true
        end
    end
    return false
end

local function valueArrayExistsInTable_ForCompMap(tbl, val)
    for key, value in pairs(tbl) do
        if val[1] == value[1] and val[2] == value[2] and val[3] == value[3] and val[4] == value[4] and val[5] == value[5] and val[6] == value[6] then
            return true
        end
    end
    return false
end

local function valueArrayExistsInTable_ForCompFinalsInCompMap(tbl, val)
    for key, value in pairs(tbl) do
        if val[1] == value[4] and val[2] == value[5] and val[3] == value[6] then
            return true
        end
    end
    return false
end

local function compare_stadium_names(a,b)
	return string.lower(a[2]) < string.lower(b[2]) -- {stadium_ID, stadium_name, stadium_path} triplets, compare names
end

local function merge_maps()
    local idx = 1
    clear_table(all_stadiums_map)

    -- for key, row in pairs(team_assignment_map) do
        -- if row[1] ~= nil and row[1] ~= "" and row[2] ~= nil and row[2] ~= "" and row[3] ~= nil and row[3] ~= "" and not valueArrayExistsInTable(all_stadiums_map, row, 1) then
            -- all_stadiums_map[idx] = {row[1], row[2], row[3]}
            -- idx = idx + 1
        -- end
    -- end
	-- v1.60 - another dimension needs to be handled for teams, similar to competition_assignment_map
	for key, value in pairs(team_assignment_map) do
        for index, row in ipairs(value) do
			if row[1] ~= nil and row[1] ~= "" and row[2] ~= nil and row[2] ~= "" and row[3] ~= nil and row[3] ~= "" and not valueArrayExistsInTable(all_stadiums_map, row, 1) then
                all_stadiums_map[idx] = {row[1], row[2], row[3]}
                idx = idx + 1
            end
        end
    end
	--

    for key, value in pairs(competition_assignment_map) do
        for index, row in ipairs(value) do
			if row[1] ~= nil and row[1] ~= "" and row[2] ~= nil and row[2] ~= "" and row[3] ~= nil and row[3] ~= "" and not valueArrayExistsInTable(all_stadiums_map, row, 1) then
                all_stadiums_map[idx] = {row[1], row[2], row[3]}
                idx = idx + 1
            end
            -- see if the stadium for final match is already included ...
			if row[4] ~= nil and row[4] ~= "" and row[5] ~= nil and row[5] ~= "" and row[6] ~= nil and row[6] ~= "" and not valueArrayExistsInTable(all_stadiums_map, row, 4) then
                all_stadiums_map[idx] = {row[4], row[5], row[6]}
                idx = idx + 1
            end
        end
    end

    -- dump_table(konami_stadiums_map)
	-- log(dump_table(all_stadiums_map))

    total_stadiums = tableLength(all_stadiums_map)
    if total_stadiums > 0 then
        manual_stad_idx = 1
    end
    log(string.format("%s unique stadiums available for random selection in exhibition modes.", total_stadiums))
    table.sort(all_stadiums_map, compare_stadium_names)
end

local function load_map_txt(filename)
    local delim = ","
    
	-- map_comp_finals.txt will be optional - check if it exists first, and return immediately if it does not
	if string.lower(filename) == "map_comp_finals.txt" and not file_exists(stadiumroot .. filename) then
		log("INFO: map_comp_finals.txt not found")
		return
	end
	
    local data = assert(io.lines(stadiumroot .. filename))
    log(filename .. " found in " .. stadiumroot .. filename)

    if filename == "map_teams.txt" then
        clear_table(team_assignment_map)
    end
    if filename == "map_competitions.txt" then
        clear_table(competition_assignment_map)
    end
    
    for line in data do
        line = trim(string.gsub(line, "^\239\187\191", "")) -- removes UTF BOM bytes at the beginning of the first line in .txt file and leading/trailing whitespaces in every line
        local fields = split(line, delim)
        if #fields > 1 then
            for i=1,#fields do
                fields[i] = trim(fields[i])
            end
            if fields[1] ~= nil and fields[1] ~= "" then
                if filename == "map_teams.txt" and fields[2] ~= nil and fields[3] ~= nil and fields[4] ~= nil then
                    -- team_assignment_map[tonumber(fields[1])] = {fields[2], fields[3], fields[4]}
					-- v1.60
					if team_assignment_map[tonumber(fields[1])] ~= nil then
						if not valueArrayExistsInTable_ForTeamsMap(team_assignment_map[tonumber(fields[1])], {fields[2], fields[3], fields[4]}) then
							table.insert(team_assignment_map[tonumber(fields[1])], {fields[2], fields[3], fields[4]})
							_log(string.format(" ==> %s stadium assignment::  team_id: %s -> stadium_id: %s, stadium: %s (%s)", filename, fields[1], fields[2], fields[4], fields[3]))
						else
							_log(string.format("Skipping duplicated assignment for team %s: -> id %s, stadium %s (%s)", fields[1], fields[2], fields[4], fields[3]))
						end
					else
						team_assignment_map[tonumber(fields[1])] = { {fields[2], fields[3], fields[4]} }
						_log(string.format(" ==> %s stadium assignment::  team_id: %s -> stadium_id: %s, stadium: %s (%s)", filename, fields[1], fields[2], fields[4], fields[3]))
					end
					--
                    -- _log(string.format(" ==> %s stadium assignment::  team_id: %s -> stadium_id: %s, stadium: %s (%s)", filename, fields[1], fields[2], fields[4], fields[3]))
                end
                if filename == "map_competitions.txt" and fields[2] ~= nil and fields[3] ~= nil and fields[4] ~= nil and fields[5] ~= nil and fields[6] ~= nil and fields[7] ~= nil then
                    if competition_assignment_map[tonumber(fields[1])] ~= nil then
						if not valueArrayExistsInTable_ForCompMap(competition_assignment_map[tonumber(fields[1])], {fields[2], fields[3], fields[4], fields[5], fields[6], fields[7]}) then
								table.insert(competition_assignment_map[tonumber(fields[1])], {fields[2], fields[3], fields[4], fields[5], fields[6], fields[7]})
								_log(string.format(" ==> %s stadium assignment::  competition: %s -> stadium_id: %s, stadium: %s (%s) (finals: %s %s (%s))", filename, fields[1], fields[2], fields[4], fields[3], fields[5], fields[7], fields[6]))
						else
							_log(string.format("Skipping duplicated assignment for competition %s: -> id %s, stadium %s (%s) (finals: %s %s (%s))", fields[1], fields[2], fields[4], fields[3], fields[5], fields[7], fields[6]))
						end
                        
                    else
						competition_assignment_map[tonumber(fields[1])] = { {fields[2], fields[3], fields[4], fields[5], fields[6], fields[7]} }
						_log(string.format(" ==> %s stadium assignment::  competition: %s -> stadium_id: %s, stadium: %s (%s) (finals: %s %s (%s))", filename, fields[1], fields[2], fields[4], fields[3], fields[5], fields[7], fields[6]))
                    end
                    -- _log(string.format(" ==> %s stadium assignment::  competition: %s -> stadium_id: %s, stadium: %s (%s) (finals: %s %s (%s))", filename, fields[1], fields[2], fields[4], fields[3], fields[5], fields[7], fields[6]))
                end
				if filename == "map_comp_finals.txt" then
					-- handle new map_comp_finals.txt here - new lua table will not be created for comp_finals, competition_assignment_map will be updated accordingly (update or insert, as needed)
					if competition_assignment_map[tonumber(fields[1])] ~= nil then
						-- this compID exists in competition_assignment_map (assignments from "map_competitions.txt") - see if this map_comp_finals entry is truly a new entry or repeated one
						if not valueArrayExistsInTable_ForCompFinalsInCompMap(competition_assignment_map[tonumber(fields[1])], {fields[2], fields[3], fields[4]}) then
							-- this stadium does not seem to be mentioned yet as finals venue for this competition ... let's add it to the existing collection for this compID
							table.insert(competition_assignment_map[tonumber(fields[1])], {"", "", "", fields[2], fields[3], fields[4]})
							_log(string.format(" ==> %s stadium assignment::  competition %s finals (append): -> stadium_id: %s, stadium: %s (%s)", filename, fields[1], fields[2], fields[4], fields[3]))
						else
							-- this stadium is already mentioned at least once as finals venue for this competition ... skip it
							_log(string.format("Skipping duplicated assignment for competition %s finals (from map_comp_finals): -> stadium_id: %s, stadium: %s (%s)", fields[1], fields[2], fields[4], fields[3]))
						end
					else
						-- this compID does not exist yet in competition_assignment_map ... add new first entry
						competition_assignment_map[tonumber(fields[1])] = { {"", "", "", fields[2], fields[3], fields[4]} }
						_log(string.format(" ==> %s stadium assignment::  competition %s finals (new): -> stadium_id: %s, stadium: %s (%s)", filename, fields[1], fields[2], fields[4], fields[3]))
					end
				end
            end
        end
    end
end

local function has_value(tab, val)
    for index, value in ipairs(tab) do
        if value == val then
            return true
        end
    end
    return false
end

local function get_random_stad_all(n)
    log("last_choice was: " .. tostring(last_choice))
    last_choice = last_choice or math.random(1,n)
    log("last_choice now: " .. tostring(last_choice))
    return last_choice
end

local function get_random_stad_teams(n)
    log("last_choice_teams was: " .. tostring(last_choice_teams))
    last_choice_teams = last_choice_teams or math.random(1,n)
    log("last_choice_teams now: " .. tostring(last_choice_teams))
    return last_choice_teams
end

local function get_random_idx(n, usedIndices)
	-- we don't want endless loop ...
	if tableLength(usedIndices) >= n then -- are all available indices used already? do not use #usedIndices - seems to be counting each element twice :/
		return nil
    end
	
	local rnd_idx = math.random(1, n)
	while usedIndices[rnd_idx] == true
	do
		rnd_idx = math.random(1, n) -- loop until yet-unused index is generated
	end
	return rnd_idx
end

local function get_random_stad_comp(ctx, tid, n)
	-- could have been done more intelligently, but it would break too much of the existing code ...
	if ctx.match_info == 53 then -- finals match? make random selections until we hit stadium for finals
		log("last_choice_comp_finals was: " .. tostring(last_choice_comp_finals))
		local usedIndices = {}
		local rnd_idx = nil
		local valid_finals_stadium = false
		repeat
			rnd_idx = get_random_idx(n, usedIndices) -- return yet-unused random index in range 1-n
			-- does it point to an entry that HAS final stadium assigned?
			if rnd_idx then -- is rnd_idx not nil? then we got yet-unused stadium row - but is it defined for finals?
				-- path 6, name 5, id 4
				valid_finals_stadium = competition_assignment_map[tid][rnd_idx][6] ~= "" and competition_assignment_map[tid][rnd_idx][5] ~= "" and competition_assignment_map[tid][rnd_idx][4] ~= ""
				if not valid_finals_stadium then
					-- if not, mark current random index as used and get another one
					usedIndices[rnd_idx] = true  -- table used as set - actual value is stored in key, value is dummy (true in this case, for easy element participation check) 
				end
			else
				break -- all indices have been exhausted, none were pointing to a finals stadium
			end
		until (valid_finals_stadium == true)
		last_choice_comp_finals = last_choice_comp_finals or rnd_idx -- either valid index or nil ... (or operator is vital here too - stadium is basically selected twice (2nd time - at kick-off button) so it prevents the 1st random selection from being re-rolled during 2nd call)
		log("last_choice_comp_finals now: " .. tostring(last_choice_comp_finals))
		return last_choice_comp_finals
	else -- pre-finals match, same procedure ... until we hit stadium for pre-finals stage
		log("last_choice_comp was: " .. tostring(last_choice_comp))
		local usedIndices = {}
		local rnd_idx = nil
		local valid_stadium = false
		repeat
			rnd_idx = get_random_idx(n, usedIndices) -- return yet-unused random index in range 1-n
			-- does it point to an entry that HAS stadium for pre-finals stage assigned?
			if rnd_idx then -- is rnd_idx not nil? then we got yet-unused stadium row - but is it defined for finals?
				-- path 3, name 2, id 1
				valid_stadium = competition_assignment_map[tid][rnd_idx][3] ~= "" and competition_assignment_map[tid][rnd_idx][2] ~= "" and competition_assignment_map[tid][rnd_idx][1] ~= ""
				-- log("valid stadium: " ..  tostring(valid_stadium))
				if not valid_stadium then
					-- if not, mark current random index as used and get another one
					usedIndices[rnd_idx] = true  -- table used as set - actual value is stored in key, value is dummy (true in this case, for easy element participation check 
				end
			else
				break -- all indices have been exhausted, none were pointing to a pre-finals stadium
			end
		until (valid_stadium == true)
		last_choice_comp = last_choice_comp or rnd_idx -- either valid index or nil ... (or operator is vital here too - stadium is basically selected twice (2nd time - at kick-off button) so it prevents the 1st random selection from being re-rolled during 2nd call, i.e. it keeps the first selection)
		log("last_choice_comp now: " .. tostring(last_choice_comp))
		return last_choice_comp
	end
    
end

local function select_random_stadium_source(ctx)
    local tid = tonumber(ctx.tournament_id)
    
    --select random stadium for exhibition mode
    random_num_exhib = nil
    if tid == 65535 and rnd_server_or_cpk == 1 then
    -- if tid == 65535 and ( rnd_server_or_cpk == 1 or rnd_server_or_cpk == 2) then
        local tblLen = tableLength(all_stadiums_map)
        if tblLen == 1 then
            random_num_exhib = 1
        else
            random_num_exhib = get_random_stad_all(tblLen) 
            _log(string.format("Selecting random stadium from stadium server for exhibition mode: %s (%s) ", all_stadiums_map[random_num_exhib][1], all_stadiums_map[random_num_exhib][2]))
        end
    end
  
    --now for competiton modes ... whenever game selects new home_team, pick a random index of a stadium to be used on competition level (e.g. if multiple stadiums are assigned to one competition_ID)
    random_num_comp = nil
    if tid < 65535 and competition_assignment_map[tid] ~= nil then
		_log(string.format("tid: %s competition_assignment_map[tid]: %s, len: %s", tostring(tid), nil2str(competition_assignment_map[tid]), tostring(#competition_assignment_map[tid])))
        if #competition_assignment_map[tid] == 1 then
            -- if there's only one stadium assigned to competition, set "random" number to 1
            random_num_comp = 1 
        else
            -- if there are more stadiums, select one index randomly
            random_num_comp = get_random_stad_comp(ctx, tid, #competition_assignment_map[tid])
            -- this one CAN return nil (if all stadiums assigned to tid are for finals, yet this is pre-finals match ... or vice versa - fall back to selecting 1st entry (although empty) - latter code will gracefully fall back to using default game-selected stadium
			if random_num_comp == nil then
				random_num_comp = 1
				_log("Random stadium for competition ID " .. tostring(tid) .. " is temporarily set to 1 (due no finals stadiums found for finals match or no pre-finals stadiums found for pre-finals match - game should fall back to using one of the deafult in-game stadiums")
			end
			_log("Selecting random stadium for competition ID " .. tostring(tid) .. ": Stadium no. " .. tostring(random_num_comp) .. " (from " .. tostring(#competition_assignment_map[tid]) .. " stadium(s) available)")
        end
    end
	
	-- and for any mode, should home team have more than one stadium assigned - if stadium selection falls to map_teams.txt content, then only one of the available stadiums needs to be selected randomly
	random_num_team = nil
	if ctx.home_team and team_assignment_map[ctx.home_team] ~= nil then
		if #team_assignment_map[ctx.home_team] == 1 then
			-- if there's only one stadium assigned to the team, set "random" number to 1
            random_num_team = 1
		else
			-- if there are more stadiums, select one index randomly
			random_num_team = get_random_stad_teams(#team_assignment_map[ctx.home_team])  
			_log("Selecting random stadium for team ID " .. tostring(ctx.home_team) .. ": Stadium no. " .. tostring(random_num_team) .. " (from " .. tostring(#team_assignment_map[ctx.home_team]) .. " stadium(s) available)")
		end
	end

end



local function teams_selected(ctx, team_id)
    local tid = tonumber(ctx.tournament_id)
	
	-- reset ctx.stadium_server
    ctx.stadium_server = nil

    -- set_stadium event is called multiple times ... stadium_switched = false would be better initialized when both team ID's are known???
	stadium_switched = false
	stadium_name_switched = false
	last_choice = nil
	last_choice_teams = nil
	last_choice_comp = nil
	last_choice_comp_finals = nil
	
	if manual_selection_status == "Off" then
		manual_selection_status_info = "Modo de selección: asignación automática"
		manual_stad_info = string.format("Estadio seleccionado manualmente: Ninguno")
		current_stadium_preview_path = nil
	end

    -- if in-game stadium settings for exhibition modes are set to 'Random', then decide where to choose the stadium from - stadium server or .cpk
    --[[
    math.randomseed(os.time())
    rnd_server_or_cpk = math.random(1, 100)
    if rnd_server_or_cpk % 2 == 0 then
        rnd_server_or_cpk = 1 -- if even number, use stadium from stadium server
    else
        rnd_server_or_cpk = 2 -- otherwise if odd, use stadium from .cpk
    end
    ]]--
    -- newer approach, sider 5.1.4 - easier for getting Konami stadium previews, all Konami stadiums must be assigned in map_competitions.txt to ID 0
    rnd_server_or_cpk = 1 -- always use stadium server as a source, Konami's stadiums are listed too
    _log("If 'Random' in-game setting will be used in exhibition modes:: Source of random stadiums - either stadium server (1) or from .cpk (2): " .. tostring(rnd_server_or_cpk) )
  
end


local function is_it_stadium_file(filename)
    filename = string.lower(filename)

    if
        string.match(filename, "asset\\model\\bg\\common") or -- sweeping match of every gfx-related common asset for stadiums (too generic, perhaps??)
        string.match(filename, "asset\\model\\bg\\st%d%d%d") or -- every stadium-specific asset (stXXX)

        -- adboards? better to avoid them ... only allow custom config.xml database, for stadiums which want to remove adboards completely via blank template
		string.match(filename, "common\\bg\\model\\bg\\bill\\config\\config%.xml") or -- adboards config database

        string.match(filename, "common\\bg\\model\\bg\\draw_parameter") or
        string.match(filename, "common\\bg\\model\\bg\\tv") or
        string.match(filename, "common\\bg\\model\\bg\\bill") or

        -- stadium names mentioned in commentary
        string.match(filename, "%a%a%a\\sound\\awb\\50_main%.awb") or

        string.match(filename, "common\\demo\\fixdemo") or
        string.match(filename, "common\\demo\\fixdemoobj") or
        string.match(filename, "common\\demo\\light") or
        string.match(filename, "common\\demo\\mob") or
        string.match(filename, "common\\demo\\prop") or

        string.match(filename, "common\\render\\model\\bg\\hit\\stadium")
    then
        return true
    else
        return false
    end
end

local function which_stadium_preview_tex(filename)
    return string.match(string.lower(filename), "common\\render\\thumbnail\\stadium\\(st%d+%.dds)")
end

local function get_stadium_preview_path(stadium_info)
    if stadium_info then
        local stadium_id = stadium_info[1]
        local stadium_path = stadium_info[3]
		-- log("Overlay preview path: " .. string.format("%s%s\\common\\render\\thumbnail\\stadium\\st%03d.dds", stadiumroot, stadium_path, stadium_id))
        return string.format("%s%s\\common\\render\\thumbnail\\stadium\\st%03d.dds", stadiumroot, stadium_path, stadium_id)
    end
end

local function get_new_stadium_path(ctx)
    local tid = tonumber(ctx.tournament_id)
    local stadium_path
    local custom_st_id
    local custom_name
    
    -- if tid and not ctx.is_replay_gallery then
    if tid then
		if tid == 65535 then
			if manual_selection_status == "On" and manual_stad_idx then
				-- manual selection from overlay trumps everything
				stadium_path = all_stadiums_map[manual_stad_idx][3]
				custom_name = all_stadiums_map[manual_stad_idx][2]
				custom_st_id = all_stadiums_map[manual_stad_idx][1]
			elseif rnd_server_or_cpk and rnd_server_or_cpk == 1 and ctx.stadium_choice == 65534 and random_num_exhib ~= nil then
				-- special case for exhibition mode with random stadium selection enabled (stadium from stadium server (rnd_server_or_cpk == 1) ...
				stadium_path = all_stadiums_map[random_num_exhib][3]
				custom_name = all_stadiums_map[random_num_exhib][2]
				custom_st_id = all_stadiums_map[random_num_exhib][1]
			elseif rnd_server_or_cpk and rnd_server_or_cpk == 2 and ctx.stadium_choice == 65534 then
                return -- just leave, random .cpk stadium has been selected in exhibition mode
			elseif ctx.stadium_choice == 65533 then
				if ctx.home_team ~= nil and team_assignment_map[ctx.home_team] ~= nil then 
					-- individual home team assignments from map_teams.txt will be used here
					-- stadium_path = team_assignment_map[ctx.home_team][3]
					-- custom_name = team_assignment_map[ctx.home_team][2]
					-- custom_st_id = team_assignment_map[ctx.home_team][1]
					-- v1.60
					stadium_path = team_assignment_map[ctx.home_team][random_num_team][3]
					custom_name = team_assignment_map[ctx.home_team][random_num_team][2]
					custom_st_id = team_assignment_map[ctx.home_team][random_num_team][1]
				end
			end
		else
			if manual_selection_status == "On" and manual_stad_idx then
				-- manual selection from overlay trumps everything
				stadium_path = all_stadiums_map[manual_stad_idx][3]
				custom_name = all_stadiums_map[manual_stad_idx][2]
				custom_st_id = all_stadiums_map[manual_stad_idx][1]
			
			elseif competition_assignment_map[tid] == nil and team_assignment_map[ctx.home_team] ~= nil then -- and has_value(teams_with_cpk_homegrounds, ctx.home_team) == false then
				-- this particular competiton mode does not have the stadium assigned via map_competitions.txt
				-- .. individual home team assignments from map_teams.txt will be used here, if the team does not already ...
				-- stadium_path = team_assignment_map[ctx.home_team][3]
				-- custom_name = team_assignment_map[ctx.home_team][2]
				-- custom_st_id = team_assignment_map[ctx.home_team][1]
				-- v1.60
				stadium_path = team_assignment_map[ctx.home_team][random_num_team][3]
				custom_name = team_assignment_map[ctx.home_team][random_num_team][2]
				custom_st_id = team_assignment_map[ctx.home_team][random_num_team][1]

			elseif competition_assignment_map[tid] ~= nil and team_assignment_map[ctx.home_team] ~= nil and has_value(override_competitions, tid) and ctx.match_info ~= 53 then
				-- this particular competiton mode HAS the stadium assigned via map_competitions.txt,
				-- .. but it is also listed in override_competitions, 
				-- .. AND it is not FINAL match (e.g. in league cup, which may possibly be held at neutral ground - and having stadium assigned already in map_competitions.txt), 
                -- .. therefore, individual home team assignments from map_teams.txt will be used here again
				-- stadium_path = team_assignment_map[ctx.home_team][3]
				-- custom_name = team_assignment_map[ctx.home_team][2]
				-- custom_st_id = team_assignment_map[ctx.home_team][1]
				-- v1.60
				stadium_path = team_assignment_map[ctx.home_team][random_num_team][3]
				custom_name = team_assignment_map[ctx.home_team][random_num_team][2]
				custom_st_id = team_assignment_map[ctx.home_team][random_num_team][1]
				
			else
				-- nothing else but possible competition assignment in map_competitions.txt
				-- .. stadium from assignment from map_competitions.txt will be used, if there is any
				if tid < 65535 and competition_assignment_map[tid] ~= nil then
					if ctx.match_info == 53 and competition_assignment_map[tid][random_num_comp][4] ~= "" and competition_assignment_map[tid][random_num_comp][5] ~= "" and competition_assignment_map[tid][random_num_comp][6] ~= "" then
						-- is this the final match of a competition (53)?
						stadium_path = competition_assignment_map[tid][random_num_comp][6] -- use the stadium assigned for final match
						custom_name = competition_assignment_map[tid][random_num_comp][5]
						custom_st_id = competition_assignment_map[tid][random_num_comp][4]
					elseif ctx.match_info ~= 53 and competition_assignment_map[tid][random_num_comp][1] ~= "" and competition_assignment_map[tid][random_num_comp][2] ~= "" and competition_assignment_map[tid][random_num_comp][3] ~= "" then
						stadium_path = competition_assignment_map[tid][random_num_comp][3]  -- use the regular stadium assigned for this competition
						custom_name = competition_assignment_map[tid][random_num_comp][2]
						custom_st_id = competition_assignment_map[tid][random_num_comp][1]
					end
				end
			end
		end
    --[[
    elseif ctx.is_replay_gallery and ctx.is_replay_gallery == true then
        if replay_source_stadium_map == "all_stadiums_map" then
            stadium_path = all_stadiums_map[replay_source_stadium_key][3]
            custom_name = all_stadiums_map[replay_source_stadium_key][2]
            custom_st_id = all_stadiums_map[replay_source_stadium_key][1]
        elseif replay_source_stadium_map == "team_assignment_map" then
            stadium_path = team_assignment_map[replay_source_stadium_key][3]
            custom_name = team_assignment_map[replay_source_stadium_key][2]
            custom_st_id = team_assignment_map[replay_source_stadium_key][1]
        end
    ]]--
    end
	
    return stadium_path, custom_st_id, custom_name
end

local function get_stadium_addon_config(ctx, stadium_path)
    local t = {}
    if file_exists(stadiumroot .. stadium_path .. "\\AddOn\\addon_config.ini") then
        t = load_addon_ini(stadium_path .. "\\AddOn\\addon_config.ini")
        return t
    end
end

local function get_stadium_forced_weather_config(ctx, stadium_path)
    local t = {}
    if file_exists(stadiumroot .. stadium_path .. "\\force_weather.ini") then
        t = load_force_weather_ini(stadium_path .. "\\force_weather.ini")
        return t
    end
end

local function check_for_addon_override(ctx, stadium_path, st_id, filename)
	local override_found = false
	local weatherPrefix = nil
    local weatherTypePrefix = nil
	local comp_id = tonumber(ctx.tournament_id) 
	local home_team = tonumber(ctx.home_team)
	
	if stadium_path and st_id and filename then
        
		if ctx.season == 0 and ctx.timeofday == 0 then
			weatherPrefix = "Summer_Day"
		elseif ctx.season == 0 and ctx.timeofday == 1 then
			weatherPrefix = "Summer_Night"
		elseif ctx.season == 1 and ctx.timeofday == 0 then
			weatherPrefix = "Winter_Day"
		elseif ctx.season == 1 and ctx.timeofday == 1 then
			weatherPrefix = "Winter_Night"
        end
        if ctx.weather == 0 then
            weatherTypePrefix = "Fine"
        elseif ctx.weather == 1 then
            weatherTypePrefix = "Rainy"
        elseif ctx.weather == 2 then
            weatherTypePrefix = "Snowy"
        end
		
		-- checking for possible hard-override from force_weather.ini ... it has to have priority over the manually selected values on overlay
		-- this should hopefully keep weather-based addons in sync with forced weather settings
		-- similar checks need to be made in weather conditions event ... there we will actually be forcing the forced weather settings
		if ctx.forced_weather then
			local f_season, f_timeofday
			
			f_season = ctx.season == 0 and "Summer" or "Winter"
			f_timeofday = ctx.timeofday == 0 and "Day" or "Night" -- defaults, in case either season or timeofday are omitted in .ini
			if ctx.forced_weather.Season and string.lower(ctx.forced_weather.Season) == "summer" then
				f_season = "Summer"
			elseif ctx.forced_weather.Season and string.lower(ctx.forced_weather.Season) == "winter" then
				f_season = "Winter"
			end
			if ctx.forced_weather.Time_of_day and string.lower(ctx.forced_weather.Time_of_day) == "day" then
				f_timeofday = "Day"
			elseif ctx.forced_weather.Time_of_day and string.lower(ctx.forced_weather.Time_of_day) == "night" then
				f_timeofday = "Night"
			end
			weatherPrefix = f_season .. "_" .. f_timeofday
			
			if ctx.forced_weather.Weather and string.lower(ctx.forced_weather.Weather) == "fine" then
				weatherTypePrefix = "Fine"
			elseif ctx.forced_weather.Weather and string.lower(ctx.forced_weather.Weather) == "rainy" then
				weatherTypePrefix = "Rainy"
			elseif ctx.forced_weather.Weather and string.lower(ctx.forced_weather.Weather) == "snowy" then
				weatherTypePrefix = "Snowy"
			end
			-- IMPORTANT: no need to check for forced Weather_Effects here (forced rain/snow fall) ... add-ons do not react to that option. But it must be handled in weather conditions event to enforce this weather setting
		end
		-- end check

        if ctx.stadserv_addons_list then
            -- weather-specific files for a specific stadium have priority ...
            if override_found == false and weatherPrefix and weatherTypePrefix then
                -- fine/rainy/snowy specifics first
                for idx_key, mod_name in pairs(ctx.stadserv_addons_list) do
                    if file_exists(stadiumroot .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\" .. st_id .. "\\" .. weatherPrefix .. "\\" .. weatherTypePrefix .. "\\".. filename) then
                        _log("Weather-type-specific, fixed-stadium AddOn file found: " .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\".. st_id .. "\\" .. weatherPrefix .. "\\" .. weatherTypePrefix .. "\\" .. filename )
                        stadium_path = stadium_path .. "\\AddOn\\" .. mod_name .. "\\".. st_id .. "\\" .. weatherPrefix .. "\\" .. weatherTypePrefix
                        -- _log("stadium_path (weather-type-specific): " .. stadium_path)
                        override_found = true
                        break
                    end
                end
                -- if not fine/rain/snowy, then fallback to weather-specific that considers only season and time of day
                if override_found == false then
                    for idx_key, mod_name in pairs(ctx.stadserv_addons_list) do
                        if file_exists(stadiumroot .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\" .. st_id .. "\\" .. weatherPrefix .. "\\" .. filename) then
                            _log("Weather-specific, fixed-stadium AddOn file found: " .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\".. st_id .. "\\" .. weatherPrefix .. "\\" .. filename )
                            stadium_path = stadium_path .. "\\AddOn\\" .. mod_name .. "\\".. st_id .. "\\" .. weatherPrefix
                            -- _log("stadium_path (weather-specific): " .. stadium_path)
                            override_found = true
                            break
                        end
                    end
                    -- if not weather-specific, then perhaps there is a non weather-specific version of a file for a specific stadium id?
                    if override_found == false then
                        -- fine/rainy/snowy specifics first
                        for idx_key, mod_name in pairs(ctx.stadserv_addons_list) do
                            if file_exists(stadiumroot .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\".. st_id .. "\\Any_Weather\\" .. weatherTypePrefix .. "\\".. filename) then
                                _log("Any-weather (type-specific), fixed-stadium AddOn file found: " .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\".. st_id .. "\\Any_Weather\\" .. weatherTypePrefix .. "\\".. filename )
                                stadium_path = stadium_path .. "\\AddOn\\" .. mod_name .. "\\".. st_id.. "\\Any_Weather\\" .. weatherTypePrefix
                                -- _log("stadium_path (any-weather, type-specific): " .. stadium_path)
                                override_found = true
                                break
                            end
                        end
                        -- if not fine/rain/snowy, then fallback to any-weather
                        if override_found == false then
                            for idx_key, mod_name in pairs(ctx.stadserv_addons_list) do
                                if file_exists(stadiumroot .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\".. st_id .. "\\Any_Weather\\" .. filename) then
                                    _log("Any-weather, fixed-stadium AddOn file found: " .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\".. st_id .. "\\Any_Weather\\" .. filename )
                                    stadium_path = stadium_path .. "\\AddOn\\" .. mod_name .. "\\".. st_id.. "\\Any_Weather"
                                    -- _log("stadium_path (any-weather): " .. stadium_path)
                                    override_found = true
                                    break
                                end
                            end

                            -- if not non-weather fixed-stadium file too, then perhaps there is a generic version of a file for any weather and any stadium id?
                            if override_found == false then
                                for idx_key, mod_name in pairs(ctx.stadserv_addons_list) do
                                    if file_exists(stadiumroot .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Global\\" .. filename) then
                                        _log("Global AddOn file found: " .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Global\\" .. filename )
                                        stadium_path = stadium_path .. "\\AddOn\\" .. mod_name .. "\\Global"
                                        -- _log("stadium_path (global): " .. stadium_path)
										override_found = true
                                        break
                                    end
                                end

								-- competitionID-based overrides (e.g. UCL stadium boards, etc.)
								-- home-team based version for a given competition, perhaps?
								if override_found == false then
									for idx_key, mod_name in pairs(ctx.stadserv_addons_list) do
										-- v1.55 ... but apply home team overrides only if the match IS NOT the finals of the competition - specific comp finals overrides will be attached only to the competition 
										if ctx.match_info ~= 53 and file_exists(stadiumroot .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Comp\\" .. comp_id .. "\\Team\\" .. home_team .. "\\" .. filename) then
											_log("Home-Team specific, competition-based AddOn file found: " .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Comp\\" .. comp_id .. "\\Team\\" .. home_team .. "\\" .. filename )
											stadium_path = stadium_path .. "\\AddOn\\" .. mod_name .. "\\Comp\\" .. comp_id .. "\\Team\\" .. home_team
											-- _log("stadium_path (home-team, competition-based): " .. stadium_path)
											override_found = true
											break
										end
									end
									
									-- and if not home-team competition specific version, then perhaps regular competition version?
									if override_found == false then
										for idx_key, mod_name in pairs(ctx.stadserv_addons_list) do
											-- v1.55 ... is it competition finals?
											if ctx.match_info == 53 and file_exists(stadiumroot .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Comp\\" .. comp_id .. "\\Finals\\" .. filename) then
												_log("Competition-based AddOn file for the Finals found: " .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Comp\\" .. comp_id .. "\\Finals\\" .. filename )
												stadium_path = stadium_path .. "\\AddOn\\" .. mod_name .. "\\Comp\\" .. comp_id .. "\\Finals"
												-- _log("stadium_path (competition-based, finals): " .. stadium_path)
												override_found = true
												break
											-- if not finals ...
											elseif file_exists(stadiumroot .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Comp\\" .. comp_id .. "\\" .. filename) then
												_log("Competition-based AddOn file found: " .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Comp\\" .. comp_id .. "\\" .. filename )
												stadium_path = stadium_path .. "\\AddOn\\" .. mod_name .. "\\Comp\\" .. comp_id
												-- _log("stadium_path (competition-based): " .. stadium_path)
												override_found = true
												break
											end
										end
										
                                        
                                        if override_found == false then
                                            for idx_key, mod_name in pairs(ctx.stadserv_addons_list) do
                                                if file_exists(stadiumroot .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Team\\" ..ctx.home_team .. "\\" .. filename) then
                                                    _log( ctx.home_team .. " - Global AddOn file found: " .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Team\\" .. ctx.home_team .. "\\" .. filename )
                                                    stadium_path = stadium_path .. "\\AddOn\\" .. mod_name .. "\\Team\\" ..ctx.home_team
                                                    -- _log("stadium_path (global): " .. stadium_path)
                                                    override_found = true
                                                    break
                                                end
                                            end

                                            if override_found == false then
                                                for idx_key, mod_name in pairs(ctx.stadserv_addons_list) do
                                                    if file_exists(stadiumroot .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Team\\" ..ctx.home_team .. "\\Comp\\" .. comp_id .. "\\" .. filename) then
                                                        _log( ctx.home_team .. " - Global AddOn file found: " .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Team\\" .. ctx.home_team .. "\\Comp\\" .. comp_id .. "\\" .. filename )
                                                        stadium_path = stadium_path .. "\\AddOn\\" .. mod_name .. "\\Team\\" .. ctx.home_team .. "\\Comp\\" .. comp_id .. "\\"
                                                        -- _log("stadium_path (global): " .. stadium_path)
                                                        override_found = true
                                                        break
                                                    end
                                                end

                                                if override_found == false then
                                                    for idx_key, mod_name in pairs(ctx.stadserv_addons_list) do
                                                        if file_exists(stadiumroot .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Home\\" ..ctx.home_team .. "\\" .. filename) then
                                                            _log( ctx.home_team .. " - Global AddOn file found: " .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Home\\" .. ctx.home_team .. "\\" .. filename )
                                                            stadium_path = stadium_path .. "\\AddOn\\" .. mod_name .. "\\Home\\" ..ctx.home_team
                                                            -- _log("stadium_path (global): " .. stadium_path)
                                                            override_found = true
                                                            break
                                                        end
                                                    end

                                                    
                                                    if override_found == false then
                                                        for idx_key, mod_name in pairs(ctx.stadserv_addons_list) do
                                                            if file_exists(stadiumroot .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Away\\" ..ctx.away_team .. "\\" .. filename) then
                                                                _log( ctx.away_team .. " - Global AddOn file found: " .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Away\\" .. ctx.away_team .. "\\" .. filename )
                                                                stadium_path = stadium_path .. "\\AddOn\\" .. mod_name .. "\\Away\\" ..ctx.away_team
                                                                -- _log("stadium_path (global): " .. stadium_path)
                                                                override_found = true
                                                                break
                                                            end
                                                        end

                                                        if override_found == false then
                                                            for idx_key, mod_name in pairs(ctx.stadserv_addons_list) do
                                                                if file_exists(stadiumroot .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Home\\" ..ctx.home_team .. "\\Away\\" ..ctx.away_team .. "\\" .. filename) then
                                                                    _log( ctx.away_team .. " - Global AddOn file found: " .. stadium_path .. "\\AddOn\\" .. mod_name .. "\\Home\\" ..ctx.home_team .. "\\Away\\" .. ctx.away_team .. "\\" .. filename )
                                                                    stadium_path = stadium_path .. "\\AddOn\\" .. mod_name .. "\\Home\\" ..ctx.home_team .. "\\Away\\" ..ctx.away_team
                                                                    -- _log("stadium_path (global): " .. stadium_path)
                                                                    override_found = true
                                                                    break
                                                                end
                                                            end
                                                    
                                                            -- if not stadium/pack-global file too, then perhaps there is a uber-generic version of a file for an entire stadium server?
                                                            if override_found == false then
                                                                if file_exists(stadiumroot ..  "\\Super-Global\\" .. filename) then
                                                                    _log("StadiumServer-Global AddOn file found: " .. "Super-Global\\" .. filename )
                                                                    stadium_path = "Super-Global"
                                                                    -- _log("stadium_path (super-global): " .. stadium_path)
                                                                    override_found = true
                                                                end
                                                            end
                                                        end
                                                    end
                                                end
                                            end
                                        end
									end
								end
                            end
                        end
                    end
                end
            end
        end

	end
	
	-- if override_found == true then
	-- 	_log("AddOn:: Overriden file path to be used: " .. stadium_path)
	-- end
	
	return stadium_path, override_found

end

local function make_key(ctx, filename)
    local custom_st_id
    local custom_name
    -- if ctx.is_edit_mode == nil and stadium_switched == true then -- do something only if edit mode is NOT active
    if stadium_switched == true then
        if is_it_stadium_file(filename) then -- any stadium file, but preview
            local new_stadium_path
            local custom_st_id
            local custom_name
            new_stadium_path, custom_st_id, custom_name = get_new_stadium_path(ctx)
			if new_stadium_path and custom_st_id then
            -- if new_stadium_path and custom_st_id and file_exists(stadiumroot .. new_stadium_path .. "\\" .. filename) then
				if string.match(string.lower(filename), "common\\bg\\model\\bg\\bill\\config\\config.xml") then
					if file_exists(stadiumroot .. new_stadium_path .. "\\" .. filename) then
						log("Custom adboards config.xml found: " .. new_stadium_path .. "\\" .. filename)
					end
				end
				-- let's check for possible AddOn files ...
				new_stadium_path, ovr_found = check_for_addon_override(ctx, new_stadium_path, custom_st_id, filename)
				if ovr_found == true then
					_log("AddOn:: Full overriden file path to be used: " .. new_stadium_path .. "\\" .. filename)
				end
			
				return new_stadium_path .. "\\" .. filename
            end
        end
        -- now override the preview, if the stadium has already been predetermined by the game
        local spt_fname = which_stadium_preview_tex(filename)
        if spt_fname then
            local new_stadium_path
            local custom_st_id
            local custom_name
            new_stadium_path, custom_st_id, custom_name = get_new_stadium_path(ctx)
            if new_stadium_path then
                return string.gsub(new_stadium_path .. "\\" .. filename, "\\st%d%d%d%.dds", string.format("\\st%s.dds", custom_st_id))
            end
        end
    end
end

local function get_filepath(ctx, filename, key)
    -- if ctx.is_edit_mode == nil then -- do something only if edit mode is NOT active
        if key and filename and filename ~= key then -- filename ~= key shoud ensure that it really is a file from custom repository, with modified path prefix???
            -- log(string.format("Stadium file assignment for team ID %s (competition ID %s) - %s\\%s", nil2str(ctx.home_team), nil2str(ctx.tournament_id), stadiumroot, key))
            return string.format("%s\\%s", stadiumroot, key)
        end
    -- end
end


-- local function get_stadium_data_from_konami_stadiums_map(stadium_id)
local function get_stadium_data_from_stadiums_map(stadium_id, stad_map) -- to be used only with all_stadiums_map
    -- if stadium_id then
	if stadium_id and stad_map then
        -- for key, value in pairs(konami_stadiums_map) do
		for key, value in pairs(stad_map) do
            if value[1] == string.format("%03d", stadium_id) then
                return value[2], value[3]
            end
        end
    end
end

-- this event allows only stadium_id to be changed in options!! other params are read-only (time of day/weather/weather effects/season)
-- 6-nov (Phoenix Street): con el Street encendido y en Entrenamiento libre, el estadio de la carpeta indicada se elige solo
-- (equivale a la seleccion manual F10->9->F3). Config: content\\phoenix\\estadio_auto.ini (estadio_auto=1/0 · carpeta=Phoenix\\Calle 30x20).
-- (sin pcall: en el entorno de modulos de Sider no existe). Fuera del Entrenamiento libre o con el Street apagado no toca nada y devuelve la seleccion manual a como estaba.
local street_auto_forzado, street_auto_prev_status, street_auto_prev_idx = false, nil, nil
local function street_auto_estadio(ctx)
    if ctx and ctx.sider_dir then
        local function leer(ruta) local f = io.open(ruta, "rb"); if not f then return nil end; local s = f:read("*a"); f:close(); return s end
        local base = ctx.sider_dir .. "\\"
        local ini = leer(base .. "content\\phoenix\\estadio_auto.ini") or ""
        local auto = ini:match("estadio_auto%s*=%s*(%d)")
        local carpeta = ini:match("carpeta%s*=%s*([^\r\n]+)") or "Phoenix\\Calle 30x20"
        carpeta = carpeta:gsub("%s+$", "")
        local cfg = leer(base .. "phoenix_core\\phoenix_core.json") or ""
        local street = not cfg:find('"street_activo"%s*:%s*false')
        local sonda = leer(base .. "content\\phoenix\\sonda.json") or ""
        local zona = sonda:find('"nombre"%s*:%s*"FREE_TRAINING"') ~= nil
        local quiere = (auto ~= "0") and street and zona
        if quiere then
            local idx
            for i, e in ipairs(all_stadiums_map) do if e[3] == carpeta then idx = i; break end end
            if idx then
                if not street_auto_forzado then street_auto_prev_status, street_auto_prev_idx = manual_selection_status, manual_stad_idx end
                manual_selection_status = "On"; manual_stad_idx = idx; street_auto_forzado = true
                log("[street_auto] estadio automatico: " .. carpeta)
            else
                log("[street_auto] carpeta no encontrada en la lista de estadios: " .. carpeta)
            end
        elseif street_auto_forzado then
            manual_selection_status = street_auto_prev_status or "Off"; manual_stad_idx = street_auto_prev_idx
            street_auto_forzado = false
            log("[street_auto] seleccion manual devuelta a como estaba")
        end
    end
end

local function set_stadium(ctx, options)
    street_auto_estadio(ctx)
    random_num_exhib = nil
    random_num_comp = nil
	random_num_team = nil

    -- if ctx.is_edit_mode == nil and ctx.tournament_id then
	if ctx.tournament_id then
        local tid = tonumber(ctx.tournament_id)
        local custom_st_id
        local custom_name
        local custom_path
        local map_source
        
		if tid == 65535 then
			if manual_selection_status == "On" and manual_stad_idx then
				-- manual selection from overlay trumps everything
				custom_st_id = all_stadiums_map[manual_stad_idx][1]
				custom_name = all_stadiums_map[manual_stad_idx][2]
                custom_path = all_stadiums_map[manual_stad_idx][3]
				stadium_switched = true
				log("Switching to stadium with ID " .. custom_st_id .. " (exhibition mode, manual selection via overlay)")
            elseif ctx.stadium_choice and ctx.stadium_choice ~= 65533 and ctx.stadium_choice ~= 65534 then
				log("ctx.stadium_choice: " .. nil2str(ctx.stadium_choice))
				-- exhibition modes, and the choice is neither "Home stadium" nor "Random" (i.e. - the choice is fixed stadium)
				-- therefore, do not change the stadium
				return
			elseif ctx.stadium_choice and ctx.stadium_choice == 65533 then
				-- handle home team assignment via map_teams.txt
				if manual_selection_status == "On" and manual_stad_idx then
					-- manual selection from overlay trumps everything
					custom_st_id = all_stadiums_map[manual_stad_idx][1]
					custom_name = all_stadiums_map[manual_stad_idx][2]
                    custom_path = all_stadiums_map[manual_stad_idx][3]
					stadium_switched = true
					log("Switching to stadium with ID " .. custom_st_id .. " (exhibition mode, manual selection via overlay)")
				elseif ctx.home_team ~= nil and team_assignment_map[ctx.home_team] ~= nil then
					-- custom_st_id = team_assignment_map[ctx.home_team][1]
					-- custom_name = team_assignment_map[ctx.home_team][2]
                    -- custom_path = team_assignment_map[ctx.home_team][3]
					--v1.60
					select_random_stadium_source(ctx)
					custom_st_id = team_assignment_map[ctx.home_team][random_num_team][1]
					custom_name = team_assignment_map[ctx.home_team][random_num_team][2]
                    custom_path = team_assignment_map[ctx.home_team][random_num_team][3]
					--
					stadium_switched = true
					map_source = "exhibition mode, team-level assignment"
					log("Switching to stadium with ID " .. custom_st_id .. " (exhibition mode, team-level assignment)")
				end
			elseif ctx.stadium_choice and ctx.stadium_choice == 65534 then
				-- exhibition modes, the choice is "Random"
				-- change the stadium depending on the value of rnd_server_or_cpk
				if rnd_server_or_cpk then
					if rnd_server_or_cpk == 2 then
						-- if .cpk is the source (rnd_server_or_cpk == 2), do not change the stadium
                        select_random_stadium_source(ctx)
						map_source = "exhibition mode, random assignment (stadium from .cpk selected)"
						manual_selection_status_info = string.format("Selection mode: %s)", map_source)
                        return
					else
						-- Random setting in exhibition modes, stadium server is the source - always allow for random selection, even if the team has exclusive .cpk home ground
						select_random_stadium_source(ctx)
                        custom_st_id = all_stadiums_map[random_num_exhib][1]
                        custom_name = all_stadiums_map[random_num_exhib][2]
                        custom_path = all_stadiums_map[random_num_exhib][3]
                        stadium_switched = true
                        map_source = "exhibition mode, random assignment"
                        log("Switching to stadium with ID " .. custom_st_id .. " (" .. custom_name .. ")" .. " exhibition mode, random assignment")
					end
				end
			end
        else			
			if manual_selection_status == "On" and manual_stad_idx then
				-- manual selection from overlay trumps everything
				custom_st_id = all_stadiums_map[manual_stad_idx][1]
				custom_name = all_stadiums_map[manual_stad_idx][2]
                custom_path = all_stadiums_map[manual_stad_idx][3]
				stadium_switched = true
				log("Switching to stadium with ID " .. custom_st_id .. " (competition mode, manual selection via overlay)")
			elseif ctx.home_team ~= nil and competition_assignment_map[tid] == nil and team_assignment_map[ctx.home_team] ~= nil then
				-- custom_st_id = team_assignment_map[ctx.home_team][1]
				-- custom_name = team_assignment_map[ctx.home_team][2]
                -- custom_path = team_assignment_map[ctx.home_team][3]
				-- v1.60
				select_random_stadium_source(ctx)
				custom_st_id = team_assignment_map[ctx.home_team][random_num_team][1]
				custom_name = team_assignment_map[ctx.home_team][random_num_team][2]
				custom_path = team_assignment_map[ctx.home_team][random_num_team][3]
				--
				stadium_switched = true
				map_source = "competition mode, team-level assignment"
				log("Switching to stadium with ID " .. custom_st_id .. " (competition mode, team-level assignment)")
			elseif ctx.home_team ~= nil and competition_assignment_map[tid] ~= nil and team_assignment_map[ctx.home_team] ~= nil and has_value(override_competitions, tid) == true and ctx.match_info ~= 53 then
				-- comptition assignment will be overriden by team assignment
				-- custom_st_id = team_assignment_map[ctx.home_team][1]
				-- custom_name = team_assignment_map[ctx.home_team][2]
                -- custom_path = team_assignment_map[ctx.home_team][3]
				-- v1.60
				select_random_stadium_source(ctx)
				custom_st_id = team_assignment_map[ctx.home_team][random_num_team][1]
				custom_name = team_assignment_map[ctx.home_team][random_num_team][2]
				custom_path = team_assignment_map[ctx.home_team][random_num_team][3]
				--
				stadium_switched = true
				map_source = "competition mode, team-level assignment (overrides competition-level assignment)"
				log("Switching to stadium with ID " .. custom_st_id .. " (competition mode, team-level assignment (overrides competition-level assignment))")
			elseif tid < 65535 and competition_assignment_map[tid] ~= nil then
				select_random_stadium_source(ctx)
				if ctx.match_info == 53 and competition_assignment_map[tid][random_num_comp][4] ~= "" and competition_assignment_map[tid][random_num_comp][5] ~= "" and competition_assignment_map[tid][random_num_comp][6] ~= "" then
					-- is this the final match of a competition (53)?
					custom_st_id = competition_assignment_map[tid][random_num_comp][4]
					custom_name = competition_assignment_map[tid][random_num_comp][5]
                    custom_path = competition_assignment_map[tid][random_num_comp][6]
					stadium_switched = true
					map_source = "competition mode (final match), competition-level assignment"
					log("Switching to stadium with ID " .. custom_st_id .. " (competition mode (final match of the competition), competition-level assignment)")
				else 
					-- if has_value(teams_with_cpk_homegrounds, ctx.home_team) == false and competition_assignment_map[tid][random_num_comp][1] ~= "" and competition_assignment_map[tid][random_num_comp][2] ~= "" and competition_assignment_map[tid][random_num_comp][3] ~= "" then
					if ctx.match_info ~= 53 and competition_assignment_map[tid][random_num_comp][1] ~= "" and competition_assignment_map[tid][random_num_comp][2] ~= "" and competition_assignment_map[tid][random_num_comp][3] ~= "" then
						-- competition mode, pre-finals, no exclusive .cpk home ground -> change the stadium
						custom_st_id = competition_assignment_map[tid][random_num_comp][1]
						custom_name = competition_assignment_map[tid][random_num_comp][2]
                        custom_path = competition_assignment_map[tid][random_num_comp][3]
						stadium_switched = true
						map_source = "competition mode, competition-level assignment"
						log("Switching to stadium with ID " .. custom_st_id .. " (competition mode (pre-finals stage of the competition), competition-level assignment)")
					end
                end
            end
		
		end

        if stadium_switched == true then
            -- get_stadium_config(ctx)
            ctx.stadserv_addons_list = get_stadium_addon_config(ctx, custom_path) or {}
			ctx.forced_weather = get_stadium_forced_weather_config(ctx, custom_path) or {}
			if ctx.forced_weather and ctx.forced_weather.Time_of_day and (string.lower(ctx.forced_weather.Time_of_day) ~= "day" and string.lower(ctx.forced_weather.Time_of_day) ~= "night" ) then
				_log("forced_weather: unknown setting for Time_of_day: " .. nil2str(ctx.forced_weather.Time_of_day) .. " ... weather settings will not be forced.")
				ctx.forced_weather = {}
			end
			if ctx.forced_weather and ctx.forced_weather.Weather and (string.lower(ctx.forced_weather.Weather) ~= "fine" and string.lower(ctx.forced_weather.Weather) ~= "rainy" and string.lower(ctx.forced_weather.Weather) ~= "snowy" ) then
				_log("forced_weather: unknown setting for Weather: " .. nil2str(ctx.forced_weather.Weather) .. " ... weather settings will not be forced.")
				ctx.forced_weather = {}
			end
			if ctx.forced_weather and ctx.forced_weather.Season and (string.lower(ctx.forced_weather.Season) ~= "summer" and string.lower(ctx.forced_weather.Season) ~= "winter" ) then
				_log("forced_weather: unknown setting for Season: " .. nil2str(ctx.forced_weather.Season) .. " ... weather settings will not be forced.")
				ctx.forced_weather = {}
			end
			--log("After get_stadium_forced_weather_config - forced weather dump: ")
            --for k, v in pairs(ctx.forced_weather) do
            --    log(k.. ": " .. v)
            --end
            --log("After get_stadium_addon_config - addons_list dump: ")
            --for k, v in pairs(ctx.stadserv_addons_list) do
            --    log(k.. ": " .. v)
            --end
            if manual_selection_status == "Off" then
				if custom_name then
					manual_selection_status_info = string.format("Modo de selección: asignación automática (using stadium %s, %s)", custom_name, map_source)
				elseif not custom_name then
					manual_selection_status_info = string.format("Selection mode: %s)", map_source)
				end
            end
			if custom_st_id and custom_path then
				_stadium_info[1] = custom_st_id
				_stadium_info[2] = custom_name
				_stadium_info[3] = custom_path
				current_stadium_preview_path = get_stadium_preview_path(_stadium_info)
				_log("current_stadium_preview_path: " .. current_stadium_preview_path)
			end
            return { stadium = tonumber(custom_st_id) } -- Stadium id of the served stadium
        else
			current_stadium_preview_path = nil
            if manual_selection_status == "Off" then
				if ctx.stadium_choice and ctx.stadium_choice == 65534 and rnd_server_or_cpk and rnd_server_or_cpk == 2 then
					manual_selection_status_info = string.format("Selection mode: %s)", map_source)
				else
					manual_selection_status_info = string.format("Modo de selección: asignación automática (using stadium: %s)", "Estadio personalizado no asignado, volviendo al predeterminado")
				end
			end
        end
    end
end


local function change_gfx_preview_fallback(ctx)
    -- bound to after_set_conditions event -- use it only for matches where .cpk stadiums are used, to override potentialy old data
    local custom_name
    local custom_path
    local tid = ctx.tournament_id

	--- export custom path, name and id to context
	ctx.stadium_server = nil
    local custom_st_id
    custom_path, custom_st_id, custom_name = get_new_stadium_path(ctx)
    if custom_name and custom_st_id and custom_path then
        ctx.stadium_server = {['path'] = custom_path, ['id'] = custom_st_id, ['name'] = custom_name}
		log(string.format("ctx.stadium_server: {path=%s, id=%s, name=%s}", custom_path, custom_st_id, custom_name))
    end
    ----------

    -- if tid == 65535 and ctx.stadium_choice and ctx.stadium_choice ~= 65533 and ctx.stadium_choice ~= 65534 then
	if tid == 65535 and ctx.stadium_choice and ctx.stadium_choice ~= 65533 and ctx.stadium_choice ~= 65534 and manual_selection_status == "Off" then
        -- exhibition modes, and the choice is neither "Home stadium" nor "Random" (i.e. - the choice is fixed stadium)
        custom_name, custom_path = get_stadium_data_from_stadiums_map(ctx.stadium_choice, all_stadiums_map)
		custom_st_id = ctx.stadium_choice
        if custom_name and custom_path then
            current_stadium_preview_path = string.format("%s%s\\common\\render\\thumbnail\\stadium\\st%03d.dds", stadiumroot, custom_path, ctx.stadium_choice)
            manual_selection_status_info = string.format("Selection mode: exhibition mode (%s: %s)", "Fixed stadium from in-game menu", custom_name)
        else
            manual_selection_status_info = string.format("Selection mode: exhibition mode (using stadium: %s)", "Fixed stadium from in-game menu")
            current_stadium_preview_path = nil
        end
	elseif tid == 65535 and ctx.stadium_choice and ctx.stadium_choice == 65533 and team_assignment_map[ctx.home_team] ~= nil and manual_selection_status == "Off" then
		-- exhibition modes, the choice is "Home", and the team is listed in map_teams.txt
		-- custom_name, custom_path = get_stadium_data_from_stadiums_map(ctx.stadium, all_stadiums_map) 
		-- unfortunately, all_stadiums_map does not guarantee unique lookup per stadium_id any more (it used to, when it contained only default Konami stadiums)
		custom_path, _, custom_name = get_new_stadium_path(ctx)  -- v1.21 fix
		custom_st_id = ctx.stadium_choice
        if custom_name and custom_path then
            current_stadium_preview_path = string.format("%s%s\\common\\render\\thumbnail\\stadium\\st%03d.dds", stadiumroot, custom_path, ctx.stadium)
            manual_selection_status_info = string.format("Selection mode: exhibition mode (%s: %s)", "Team-level assignment", custom_name)
        else
            manual_selection_status_info = string.format("Selection mode: exhibition mode (using stadium: %s)", "??? Exclusive .cpk homeground")
            current_stadium_preview_path = nil
        end
    
	elseif tid == 65535 and ctx.stadium_choice and ctx.stadium_choice == 65533 and manual_selection_status == "Off" then
        -- exhibition modes, the choice is "Home", but the team is not listed in map_teams.txt
        custom_name, custom_path = get_stadium_data_from_stadiums_map(ctx.stadium, all_stadiums_map)
		custom_st_id = ctx.stadium_choice
        if custom_name and custom_path then
            current_stadium_preview_path = string.format("%s%s\\common\\render\\thumbnail\\stadium\\st%03d.dds", stadiumroot, custom_path, ctx.stadium)
            manual_selection_status_info = string.format("Selection mode: exhibition mode (%s: %s)", "Default .cpk homeground", custom_name)
        else
            manual_selection_status_info = string.format("Selection mode: exhibition mode (using stadium: %s)", "Default .cpk homeground")
            current_stadium_preview_path = nil
        end


    elseif tid < 65535 and competition_assignment_map[tid] == nil and team_assignment_map[ctx.home_team] ~= nil then
        -- this particular competiton mode does not have the stadium assigned via map_competitions.txt
        -- .. individual home team assignments from map_teams.txt has been used for that team, unless the team already ...

		-- do nothing, simply return (correct data was probably displayed already)
        return

	elseif tid < 65535 and competition_assignment_map[tid] ~= nil and team_assignment_map[ctx.home_team] ~= nil and has_value(override_competitions, tid) and ctx.match_info ~= 53 then
        -- this particular competiton mode HAS the stadium assigned via map_competitions.txt,
        -- .. but it is also listed in override_competitions,
        -- .. AND it is not FINAL match (e.g. in league cup, which may possibly be held at neutral ground - and having stadium assigned already in map_competitions.txt),
        -- .. therefore, individual home team assignments from map_teams.txt has been used for that team again

		-- do nothing, simply return (correct data was probably displayed already)
        return

	-- elseif tid < 65535 and competition_assignment_map[tid] == nil then
    elseif tid < 65535 and competition_assignment_map[tid] == nil and manual_selection_status == "Off" then
        -- no assignment for home team, no assignment to competition ... one of the default .cpk stadiums was probably selected
        -- just handle the preview and name on overlay
		log('are we here?')
        --custom_name, custom_path = get_stadium_data_from_stadiums_map(ctx.stadium, all_stadiums_map)
		custom_name, custom_path = get_stadium_data_from_stadiums_map(ctx.stadium_choice, all_stadiums_map)
		custom_st_id = ctx.stadium_choice
        if custom_name and custom_path then
            current_stadium_preview_path = string.format("%s%s\\common\\render\\thumbnail\\stadium\\st%03d.dds", stadiumroot, custom_path, ctx.stadium)
			manual_selection_status_info = string.format("Selection mode: competition mode (%s: %s)", "Default .cpk stadium", custom_name)
        else
            manual_selection_status_info = string.format("Selection mode: competition mode (using stadium: %s)", "Default .cpk stadium")
            current_stadium_preview_path = nil
        end

    end
	
	-- unless already done earlier, export stadium info if known
    if not ctx.stadium_server and custom_st_id and custom_path and custom_name then
        ctx.stadium_server = {['path'] = custom_path, ['id'] = custom_st_id, ['name'] = custom_name}
        log(string.format("ctx.stadium_server: {path=%s, id=%s, name=%s}", custom_path, custom_st_id, custom_name))
    end

end


local function change_stadium_name(ctx, stadium_name, stadium_id, schedule_entry)
	local home_team = ctx.home_team
    local tournament_id = ctx.tournament_id
    local match_info = ctx.match_info
    if schedule_entry then
        log(string.format("schedule_entry: {tournament_id=%s, match_info=%s, home_team=%s, away_team=%s}",
            schedule_entry.tournament_id,
            schedule_entry.match_info,
            schedule_entry.home_team,
            schedule_entry.away_team
        ))
        -- we have match-specific information
        -- this happens, when ML displays a schedule bar
        tournament_id = schedule_entry.tournament_id
        match_info = schedule_entry.match_info
        home_team = schedule_entry.home_team
    end

    -- if ctx.is_edit_mode == nil then -- do something only if edit mode is NOT active
    if ctx.tournament_id then -- ctx.tournament_id should not be available in EDIT mode ...
		if ctx.stadium_choice and ctx.tournament_id == 65535 and not stadium_switched then
            -- if it is exhibition mode and stadium selection has not been finalized yet ==> then do nothing.
            return
        end

        local new_stadium_path
        local custom_st_id
        local new_stadium_name
		
		-- create context copy so that we can pass it to get_new_stadium_path
        -- this copy may have modified values for tournament_id, match_info and home_team (for ML schedule bar)
        local ctx_copy = {}
        for k,v in pairs(ctx) do
            ctx_copy[k] = v
        end
        ctx_copy.tournament_id = tournament_id
        ctx_copy.match_info = match_info
        ctx_copy.home_team = home_team
        
        new_stadium_path, custom_st_id, new_stadium_name = get_new_stadium_path(ctx_copy)
        if new_stadium_name ~= nil and new_stadium_name ~= "" then
            _log(string.format("switching stadium-name: %s --> %s", stadium_name, new_stadium_name))
            stadium_name_switched = true
            return new_stadium_name
        end
    end
end

local function set_weather_conditions(ctx, options)
    local changed = false
	local external_opts = nil
	
	-- initial check if season is set via overlay - WeatherConditions.lua makes decisions based on season
	if weather_Season > 0 then
        options.season = weather_Season - 1
    end

	-- delegate weather decisions to WeatherConditions module
	if file_exists(ctx.sider_dir .. "\\modules\\WeatherConditions.lua") and ctx.weather_conditions then
		_log("StadiumServer calling WeatherConditions ... ")
		external_opts = ctx.weather_conditions.set_conditions(ctx, options, "StadiumServer")
		if external_opts then
			changed = true
			options = external_opts
		end
		_log("StadiumServer finished calling WeatherConditions.")
	else
		log("WeatherConditions.lua script not found or its' weather_conditions context is not available ...")
		log("Script path: " .. ctx.sider_dir .. "\\modules\\WeatherConditions.lua")
		log("ctx.weather_conditions status: " .. nil2str(ctx.weather_conditions))
	end
	
	-- and now override options returned from WeatherConditions.lua, if non-default settings are selected via stadium-server's overlay
    if weather_TimeOfDay > 0 then
        options.timeofday = weather_TimeOfDay - 1
        changed = true
    end
    if weather_Weather > 0 then
        options.weather = weather_Weather - 1
        changed = true
    end
    if weather_WeatherEffects > 0 then
        options.weather_effects = weather_WeatherEffects - 1
        changed = true
    end
    if weather_Season > 0 then
        options.season = weather_Season - 1
        changed = true
    end
	
	-- finally, override values with forced settings from stadium's force_weather.ini file, if it exists
	if ctx.forced_weather then
		if ctx.forced_weather.Season and string.lower(ctx.forced_weather.Season) == "summer" then
			options.season = 0 --Summer
			changed = true
		elseif ctx.forced_weather.Season and string.lower(ctx.forced_weather.Season) == "winter" then
			options.season = 1 --Winter
			changed = true
		end
		
		if ctx.forced_weather.Time_of_day and string.lower(ctx.forced_weather.Time_of_day) == "day" then
			options.timeofday = 0 --Day
			changed = true
		elseif ctx.forced_weather.Time_of_day and string.lower(ctx.forced_weather.Time_of_day) == "night" then
			options.timeofday = 1 --Night
			changed = true
		end
		
		if ctx.forced_weather.Weather and string.lower(ctx.forced_weather.Weather) == "fine" then
			options.weather = 0 --Fine
			changed = true
		elseif ctx.forced_weather.Weather and string.lower(ctx.forced_weather.Weather) == "rainy" then
			options.weather = 1 --Rainy
			changed = true
		elseif ctx.forced_weather.Weather and string.lower(ctx.forced_weather.Weather) == "snowy" then
			options.weather = 2 --Snowy
			changed = true 
		end
		
		if ctx.forced_weather.Weather_Effects and string.lower(ctx.forced_weather.Weather_Effects) == "default" then
			options.weather_effects = 0 --Default
			changed = true
		elseif ctx.forced_weather.Weather_Effects and string.lower(ctx.forced_weather.Weather_Effects) == "forced" then
			options.weather_effects = 2 --Forced
			changed = true
		end
	end

    if changed then
		dump_table(options)
        return options
    end

end

local function show_overlay(ctx)
    _searching = false
    _search_string = ""
    input.set_blocked(false)
end

local function hide_overlay(ctx)
    _searching = false
    _search_string = ""
    input.set_blocked(false)
end

local opts = { image_width = 0.3, image_aspect_ratio = 1.777778 }
local function overlay_on(ctx)
    weather_text = string.format("Configuración del clima: Hora del día (%s), Tipo de clima (%s), Lluvia/nieve (%s), Estación (%s)\n",
            weather_TimeOfDay_Info[tostring(weather_TimeOfDay)],
            weather_Weather_Info[tostring(weather_Weather)],
            weather_WeatherEffects_Info[tostring(weather_WeatherEffects)],
            weather_Season_Info[tostring(weather_Season)]
    )
	
	local search_text = ""
    if _searching then
        search_text = string.format("\nBuscando: %s_", _search_string)
    end
    
	local text = string.format("version %s\nComandos de StadiumServer:\nPresiona [0] para recargar los archivos map.txt\nPresiona [DEL] para limpiar los mensajes\n\nOpciones de clima (siempre activas, excepto si está en 'Por defecto' o forzado desde force_weather.ini):\n       [3] Seleccionar hora del día\n       [4] Seleccionar tipo de clima\n       [5] Forzar lluvia/nieve\n       [6] Seleccionar estación del año\n       %s\nPresiona [9] para cambiar entre asignación automática y selección manual\n     Comandos del modo manual:\n       [PageUp]/[PageDn] Cambiar estadio anterior/siguiente\n       [7] Guardar estadio actual como favorito\n       [8] Usar estadio favorito\n       [F3] Buscar un estadio\n\n%s\n%s%s\n\n%s", version, weather_text, manual_selection_status_info, manual_stad_info, search_text, info_text)
	local image = current_stadium_preview_path
    -- if image and ctx.stadium and not file_exists(current_stadium_preview_path) then
        -- let's try with super-global folder, for Konami's .cpk stadium previews
    --    current_stadium_preview_path = string.format("%s\\Super-Global\\common\\render\\thumbnail\\stadium\\st%03d.dds", stadiumroot, ctx.stadium)
    -- end
	
	return text, image, opts
end

local function get_character_for_vkey(vkey)
    local c = string.char(vkey)
    if string.match(c, "[A-Z0-9 ]") then
        return c
    end
end

local function get_similarity_score(name, chars)
    local src = string.upper(name)
    local pos = string.find(src, chars)
    if pos then
        return (#chars)*256-pos -- very strong match: whole string found
    end
    return 0
end

local function search_for_stadium(chars)
    local best_score = 0
    local best_match
    -- break into words
    local words = {}
    for word in string.gmatch(chars, "[^ ]+") do
        words[#words + 1] = word
    end
    --log(">>>>> searching ...")
    for i, b in ipairs(all_stadiums_map) do
        local name = b[2]
        -- score full search string
        local score = get_similarity_score(name, chars)
        if score <= 0 then
            -- score words separately
            score = 0
            for _, word in ipairs(words) do
                local sc = get_similarity_score(name, word)
                score = score + sc
            end
        end
        if score > 0 then
            --log(string.format("%s: score: %d", name, score))
        end
        if score > best_score then
            best_match = i
            best_score = score
        end
    end
    --log("<<<<< search done")
    return best_match
end

local function key_up(ctx, vkey)
    if vkey == SEARCH_END_KEY or vkey == SEARCH_CANCEL_KEY then
        if _searching then
            _searching = false
            _search_string = ""
            input.set_blocked(false)
        end
    end
end

local function key_down(ctx, vkey)
	if _searching then
        -- stadium search
        local character = get_character_for_vkey(vkey)
        if vkey == SEARCH_BACKSPACE_KEY then
            _search_string = _search_string:sub(1, #_search_string-1)
        elseif character and vkey ~= SEARCH_END_KEY and vkey ~= SEARCH_CANCEL_KEY then
            _search_string = _search_string .. character
        end
        local stad_idx = search_for_stadium(_search_string)
        if stad_idx then
            manual_stad_idx = stad_idx
            manual_stad_info = string.format("Estadio seleccionado manualmente (búsqueda): %s\n", all_stadiums_map[manual_stad_idx][2])
            current_stadium_preview_path = get_stadium_preview_path(all_stadiums_map[manual_stad_idx])
        end
        -- do not process special keys, if in searching mode
        return
    end
	
    if vkey == RELOAD_MAPS_KEY then
        _log("Starting manual map files reload ... ")
        load_map_txt("map_teams.txt")
        load_map_txt("map_competitions.txt")
		load_map_txt("map_comp_finals.txt") -- always load after 'map_competitions.txt'
        merge_maps()
        manual_stad_idx = nil
        manual_stad_info = "Estadio seleccionado manualmente: Ninguno"
        if total_stadiums > 0 then
            manual_stad_idx = 1
        end
        info_text = info_text .. "map_teams.txt, map_competitions.txt (and map_comp_finals.txt, if present) reloaded\n"
        _log("Manual map files reloading finished.")
    elseif vkey == DEL_TEXT_KEY then
        info_text = ""
    elseif vkey == SWITCH_SELECTION_MODE_KEY then
        if manual_selection_status == "Off" then
            manual_selection_status = "On"
            manual_selection_status_info = "Modo de selección: manual ([PageUp]/[PageDn]"
            manual_stad_info = string.format("Estadio seleccionado manualmente: %s\n", all_stadiums_map[manual_stad_idx][2])
			current_stadium_preview_path = get_stadium_preview_path(all_stadiums_map[manual_stad_idx])
        else
            manual_selection_status = "Off"
            manual_selection_status_info = "Modo de selección: asignación automática"
            manual_stad_info = string.format("Estadio seleccionado manualmente: Ninguno")
            info_text = ""
			current_stadium_preview_path = nil
        end
    elseif vkey == PREVIOUS_STAD_KEY then
        if manual_selection_status == "On" and total_stadiums > 0 and manual_stad_idx then
            manual_stad_idx = manual_stad_idx - 1
            if manual_stad_idx < 1 then
                manual_stad_idx = total_stadiums
            end
            manual_stad_info = string.format("Estadio seleccionado manualmente: %s\n", all_stadiums_map[manual_stad_idx][2])
			current_stadium_preview_path = get_stadium_preview_path(all_stadiums_map[manual_stad_idx])
        end
    elseif vkey == NEXT_STAD_KEY then
        if manual_selection_status == "On" and total_stadiums > 0 and manual_stad_idx then
            manual_stad_idx = manual_stad_idx + 1
            if manual_stad_idx > total_stadiums then
                manual_stad_idx = 1
            end
            manual_stad_info = string.format("Estadio seleccionado manualmente: %s\n", all_stadiums_map[manual_stad_idx][2])
			current_stadium_preview_path = get_stadium_preview_path(all_stadiums_map[manual_stad_idx])
        end
	elseif vkey == SEARCH_START_KEY then
        if manual_selection_status == "On" and total_stadiums > 0 and manual_stad_idx then
            _searching = true
            _search_string = ""
            -- hard block: disable module switching and overlay toggle via keyboard
            -- this is so that we can use space keys and numbers for entering search text
            input.set_blocked(true, true)
        end
    elseif vkey == SET_AS_FAVORITE_STAD_KEY then
        if manual_selection_status == "On" and manual_stad_idx then
            if all_stadiums_map[manual_stad_idx] then
                settings["favorite_stadium"] = manual_stad_idx
                save_ini("config.ini")
                info_text = info_text .. string.format("Stadium %s saved as favorite\n", all_stadiums_map[manual_stad_idx][2])
            else
                info_text = info_text .. "Could not save favorite stadium\n"
            end
        end
    elseif vkey == USE_FAVORITE_STAD_KEY then
        if manual_selection_status == "On" and settings["favorite_stadium"] then
            if all_stadiums_map[tonumber(settings["favorite_stadium"])] then
                manual_stad_idx = settings["favorite_stadium"]
                manual_stad_info = string.format("Estadio seleccionado manualmente: %s\n", all_stadiums_map[manual_stad_idx][2])
                info_text = info_text .. string.format("Favorite stadium %s set as active\n", all_stadiums_map[manual_stad_idx][2])
				current_stadium_preview_path = get_stadium_preview_path(all_stadiums_map[manual_stad_idx])
            else
                info_text = info_text .. string.format("Could not find favorite stadium (idx: %s)\n", settings["favorite_stadium"])
            end
        end
    elseif vkey == WEATHER_TIMEOFDAY_KEY then
        weather_TimeOfDay = weather_TimeOfDay + 1
        if weather_TimeOfDay > 2 then
            weather_TimeOfDay = 0
        end
    elseif vkey == WEATHER_WEATHER_KEY then
        weather_Weather = weather_Weather + 1
        if weather_Weather > 3 then
            weather_Weather = 0
        end
    elseif vkey == WEATHER_WEATHER_EFFECTS_KEY then
        if weather_WeatherEffects == 0 then
            weather_WeatherEffects = 3
        else
            weather_WeatherEffects = 0
        end
    elseif vkey == WEATHER_SEASON_KEY then
        weather_Season = weather_Season + 1
        if weather_Season > 2 then
            weather_Season = 0
        end
    end

end

local function init(ctx)
    if stadiumroot:sub(1,1)=='.' then
        stadiumroot = ctx.sider_dir .. stadiumroot
    end
	math.randomseed(os.time())
	settings = load_ini("config.ini")
    load_map_txt("map_teams.txt")
    load_map_txt("map_competitions.txt")
	load_map_txt("map_comp_finals.txt") -- always load after 'map_competitions.txt'
    merge_maps() -- create one master-table, which is going to have one entry for every stadium assigned - either via teams or competitions or competition finals
    
    ctx.register("set_stadium", set_stadium)
    ctx.register("set_conditions", set_weather_conditions)
    ctx.register("get_stadium_name", change_stadium_name)
    ctx.register("livecpk_make_key", make_key)
    ctx.register("livecpk_get_filepath", get_filepath)
    ctx.register("set_teams", teams_selected)
    ctx.register("overlay_on", overlay_on)
    ctx.register("key_down", key_down)
	ctx.register("key_up", key_up)
    ctx.register("show", show_overlay)
    ctx.register("hide", hide_overlay)
    ctx.register("after_set_conditions", change_gfx_preview_fallback)
end

return { init = init }