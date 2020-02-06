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
local twoWeeksScore = 0

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
 
function arrow(i)
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
  
  lcd.drawRectangle(1 ,1 ,(LCD_W - 1), (LCD_H - 1))

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
  lcd.drawRectangle(20 , 20 , (LCD_W - 21), (LCD_H - 21), SOLID , 2)
  lcd.drawText( ((LCD_W / 2) - 6), (LCD_H / 4), "You scored:", BLINK)
  lcd.drawText( (LCD_W / 2), (LCD_H / 2), twoWeeksScore,  MIDSIZE )
  lcd.drawText( ((LCD_W / 2) - 6), ((LCD_H /4) * 3), "2weeks", DBLSIZE)
 end

local function blankRectangle()
   for i=20, (LCD_H - 20), 1 do
    for k=20, (LCD_W - 20), 1 do
      lcd.drawText(i, k, " " ,0)
    end
  end
end

local function scoreOutput() 
         if score == 222 then
           lcd.drawText(1, LCD_H, "You collected a total of 222, Mike will get his sticks", SMLSIZE)
           lcd.drawText((LCD_W / 4), 1, "YOU WIN!", XXLSIZE) 
           lcd.drawtext(1, (LCD_H / 4), "You have 120 Seconds to take a photo of this screen", BLINK)
           lcd.drawtext(1, (LCD_H / 4), "Please post it to the Team Blacksheep Lounge on FB", INVERS)
             for j=0, 120, 1 do
               lcd.refresh()
             end
          end
  
    
    for i=0, LCD_H, 1 do
      for k=0, LCD_W, 1 do
        lcd.drawText( i,k,"*", 0)
      end
    end
  
  blankRectangle()
  endScreen()
  lcd.refresh()
  
    for i=0, LCD_H, 1 do
       for k=0, LCD_W, 1 do
         lcd.drawText( i,k,"*", INVERS)
       end
   
  end
 blankRectangle()
 endScreen()
  
 lcd.refresh() 
 lcd.refresh()
 lcd.refresh()
 lcd.refresh()
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
  if snakeCounter < 30 then
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

  lcd.refresh()
 
  if score == 222 then
    scoreOutput()
    return 1
  end

  if check_collision() then
    scoreOutput()
    return 1
  end
  
  return 0
end

return { init=init, run=run }

