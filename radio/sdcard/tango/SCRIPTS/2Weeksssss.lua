--------------------------------------------------------------
-- Classic snake game
-- Originally by
-- 2009 Led Lab @PUC-Rio www.eluaproject.net
-- Dado Sutter
-- Ives Negreiros
-- To Benjamin
--
--
-- Edited by Finn Toogood
-- Massive thanks to Marc Stelzner for testing adn bug finding
--------------------------------------------------------------

local xMax = math.floor( LCD_W / 6 ) - 1
local yMax = math.floor( LCD_H / 8 ) - 1
local game_map = {}

local Head = {}
local Tail = {}

local highscore = 0
local size = 3
Tail.x = 1
Tail.y = 1
Head.x = Tail.x + ( size - 1 )
Head.y = Tail.y

local Food = {}
Food.x = false
Food.y = false

Head.dx = 1
Head.dy = 0
Tail.dx = Head.dx
Tail.dy = Head.dy
local direction = "right"
local score = 0
local foodCount = 0
local twoWeeksScore = -0.2
local dead = 0

local function speed()
  if getValue('sa') > -900 and getValue('sd') > -900 then
    return (20 - ((getValue('thr')+1024) / 102.4) )
  else
    return 15 
end
end

local function twoWeeks()
 twoWeeksScore = twoWeeksScore + 0.2
  if foodCount == 0 then
    return "2"
    elseif foodCount == 1 then
    return "W"
    elseif foodCount == 2 then
    return "e"
    elseif foodCount == 3 then
    return "e"
    elseif foodCount == 4 then
    return "k"
    elseif foodCount == 5 then
    foodCount = 0
    return "s"
    end
  end

local function create_food()
  Food.x, Food.y = math.random( xMax - 1), math.random( yMax - 1)
  while game_map[ Food.x ][ Food.y ] do
    Food.x, Food.y = math.random( xMax - 1 ), math.random( yMax - 1 )
  end
  game_map[ Food.x ][ Food.y ] = "food"
  lcd.drawText( Food.x * 6, Food.y * 8, twoWeeks(), 0 )
  foodCount = foodCount + 1
end

local function eat_food()
  playFile("/SCRIPTS/2weeks.wav")
  lcd.drawText( Head.x * 6, Head.y * 8, " ", 0 )
  game_map[ Head.x ][ Head.y ] = nil
  create_food()
  score = score + 1
  
end

local function check_collision()
  if Head.x < 0 or Head.x > xMax then
    return true
  elseif Head.y < 0 or Head.y > yMax then
    return true
  elseif ( ( game_map[ Head.x ][ Head.y ] ) and ( game_map[ Head.x ][ Head.y ] ~= "food" ) ) then
    return true
  end
  return false
end
 
local function arrow(i)
  if i == "right" then
    return ">"
    elseif i == "left" then
    return "<"
    elseif i == "up" then
    return "^"
    elseif i == "down" then
    return "v"
    end
  end

local function move()
if dead == 0 then
  if game_map[ Tail.x ][ Tail.y ] == "right" then
    Tail.dx = 1
    Tail.dy = 0
  elseif game_map[ Tail.x ][ Tail.y ] == "left" then
    Tail.dx = -1
    Tail.dy = 0
  elseif game_map[ Tail.x ][ Tail.y ] == "up" then
    Tail.dx = 0
    Tail.dy = -1
  elseif game_map[ Tail.x ][ Tail.y ] == "down" then
    Tail.dx = 0
    Tail.dy = 1
  end
  
  game_map[ Head.x ][ Head.y ] = direction
  Head.x = Head.x + Head.dx
  Head.y = Head.y + Head.dy

  if Head.x < 0 or Head.x > xMax or Head.y < 0 or Head.y > yMax then
    return
  elseif game_map[ Head.x ][ Head.y ] == "food" then
    eat_food()
  else
    lcd.drawText(Tail.x * 6, Tail.y * 8, " ", 16)
    game_map[ Tail.x ][ Tail.y ] = nil
    Tail.x = Tail.x + Tail.dx
    Tail.y = Tail.y + Tail.dy
  end

  lcd.drawText(Head.x * 6, Head.y * 8, arrow(direction), 0)
  end
end

local function init()
  food = false
  lcd.clear()
  size = 3
  score = 0
  Tail.x = 1
  Tail.y = 1
  Head.x = Tail.x + ( size - 1 )
  Head.y = Tail.y
  Head.dx = 1
  Head.dy = 0
  Tail.dx = Head.dx
  Tail.dy = Head.dy
  direction = "right"

  for i = 0, xMax, 1 do
    game_map[ i ] = {}
  end
  
  for i = 0, size - 1, 1 do
    game_map[ Tail.x + ( i * Tail.dx ) ][ Tail.y + ( i * Tail.dy ) ] = direction
    lcd.drawText( ( Tail.x + ( i * Tail.dx ) ) * 6, ( Tail.y + ( i * Tail.dy ) ) * 8, ">", 0 )
  end
  
  create_food()
end

local function endScreen()
		dead = 1
  lcd.drawRectangle(1 , 1 , (LCD_W - 1), (LCD_H - 1), SOLID , 2)
  lcd.drawText( ((LCD_W / 2) - 30), (LCD_H / 5), "You scored:", 0)
  lcd.drawText( ((LCD_W / 2) - 10 ), (LCD_H / 2), twoWeeksScore,  MIDSIZE )
  lcd.drawText( ((LCD_W / 2) - 25), ((LCD_H /4) * 3), "2weeks", MIDSIZE)
  lcd.refresh()
 end


local function scoreOutput() 
lcd.clear()
 
         if score == 222 then
           lcd.drawText(1, 10, "You collected a total of 222 leaks", SMLSIZE)
           lcd.drawText((LCD_W / 4), 40, "YOU WIN!", 0) 
           lcd.drawText(1, 60, "Take a photo of this screen", BLINK)
           lcd.drawText(1, 80, "Please post it to the TBS Lounge on FB", INVERS)
		else 
			endScreen()
			end		   
		 end


local snakeCounter = 0

local function run(event)
  if event == nil then
    raise("Cannot be run as a model script!")
  end
  
  if event == EVT_EXIT_BREAK then
    return 2
  end
  
  snakeCounter = snakeCounter + 1
  if snakeCounter < speed() then
    
    return 0
  end
  
  snakeCounter = 0
  local dir = direction
  if getValue('ail') > 100 and direction ~= "left" then
    dir = "right"
    Head.dx = 1
    Head.dy = 0
  end
  if getValue('ail') < -100 and direction ~= "right" then
    dir = "left"
    Head.dx = -1
    Head.dy = 0
  end
  if getValue('ele') > 100 and direction ~= "down" then
    dir = "up"
    Head.dx = 0
    Head.dy = -1
  end
  if getValue('ele') < -100 and direction ~= "up" then
    dir = "down"
    Head.dx = 0
    Head.dy = 1
  end


  direction = dir

  move()

  if score == 222 then
    scoreOutput()
    --return 1
  end

  if check_collision() then
    scoreOutput()
    --return 1
  end
  
  return 0
end

return { init=init, run=run }

