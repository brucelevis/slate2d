import "engine" for Trap, Button, Draw, Asset, Fill, Color, TileMap
import "player" for Player
import "meta" for Meta
import "random" for Random

class Title {
   nextScene { null }
   construct new(mapName) {
      Trap.print(mapName+"\n")
      _dog = Asset.create(Asset.Image, "dog", "gfx/dog.png")
      _sprites = Asset.create(Asset.Image, "sprites", "maps/tilesets/plat.gif")
      _music = Asset.create(Asset.Mod, "music", "music/frantic_-_dog_doesnt_care.it")
      _speech = Asset.create(Asset.Speech, "speech", "great job! you are a good dog!")

      _font = Asset.create(Asset.BitmapFont, "font", "gfx/good_neighbors.png")
      Asset.bmpfntSet(_font, "!\"#$\%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", 0, -1, 7, 16)

      _fixedFont = Asset.create(Asset.BitmapFont, "fixedfont", "gfx/panicbomber.png")
      Asset.bmpfntSet(_fixedFont, " !\"#$\%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", 8, 0, 8, 8)

      _fixedFontBlue = Asset.create(Asset.BitmapFont, "fixedfontblue", "gfx/panicbomber_blue.png")
      Asset.bmpfntSet(_fixedFontBlue, " !\"#$\%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", 8, 0, 8, 8)

      TileMap.load(mapName)

      Asset.loadAll()

      _rnd = Random.new()
      _rndNum = 0
      _time = 0

      _asdf = "hellooo world"
      _testRange = 1..4
      _testMap2 = {Meta: "class key", 1234: "number key", "a string": "string key", true: "bool key", _testRange: "range key", null: "a null"}
      _testMap2["thing"] = "thing2"
      _testClass = Player
      _testClass2 = Trap
      _mouseCoords = null

      Trap.sndPlay(_music, 1.0, 0.0, true)
      //Trap.sndPlay(_speech, 1.0, 0.0, false)

      _spr = Asset.createSprite(_sprites, 8, 8, 0, 0)

      _bgLayer = TileMap.layerByName("bg")
      _worldLayer = TileMap.layerByName("world")

      var objectLayer = TileMap.layerByName("objects")
      var objs = TileMap.objectsInLayer(objectLayer)
      Trap.printLn(objs)

      var tiles = TileMap.getTileProperties()
      Trap.printLn(tiles)

      var gid = TileMap.getTile(_worldLayer, 0, 20)
      Trap.printLn(gid)

      var mapProps = TileMap.getMapProperties()
      Trap.printLn(mapProps)

      var layerProps = TileMap.getLayerProperties(_worldLayer)
      Trap.printLn(layerProps)

      var len = Asset.measureBmpText(_font, "hello world")
      Trap.printLn("length of text is %(len)")
   }

   update(dt) {
      _upActive = Trap.keyActive(Button.Up)

      for (i in 0..11) {
         if (Trap.keyPressed(i, 0, 1000)) { Trap.printLn("button %(i)") }
      }

      _time = _time + dt

      _mouseCoords = Trap.mousePosition()

      _rndNum = _rnd.int()
   }

   draw(w, h) {
      Trap.printWin("window name", "contents", "some value")
      Trap.printWin("window name", "some longer text", "a")
      Trap.printWin("window name", "a pretty large key name", "a pretty large value")
      Trap.printWin("window name", "a list", [1,2,3])
      Trap.printWin("window name", "number", 1.0)
      Trap.printWin("window name", "random number", _rndNum)
      Trap.printWin("window name", "map", _testMap2["key1"])

      for (i in 0..20) {
         Trap.printWin("window name", "scrolling test", i)
      }

      Draw.clear()
      Draw.resetTransform()
      Draw.transform(h / 180, 0, 0, h / 180, 0, 0)

      Draw.mapLayer(_bgLayer)
      Draw.mapLayer(_worldLayer)

      Draw.mapLayer(_worldLayer, 4, 4, 0, 14, 8, 99)
      Draw.mapLayer(_bgLayer, 4, 4, 0, 14, 8, 99)

      Draw.setColor(Color.Stroke, 255, 0, 0, 255)
      Draw.rect(120 + (_time/80).sin * 80, 3, 16, 16, Fill.Outline)

      Draw.setColor(Color.Fill, 60, 0, 90, 255)
      Draw.setScissor(0, 0, 280, 110)
      Draw.circle(270, 100, 20, Fill.Outline)
      Draw.resetScissor()

      Draw.setColor(Color.Stroke, 0, 255, 0, 255)
      Draw.line(0, 0, 320, 180)

      Draw.setColor(Color.Stroke, 130, 140, 150, 255)
      Draw.rect(10, 30, 64, 64, Fill.Solid)

      Draw.setColor(Color.Stroke, 255, 255, 0, 255)
      Draw.circle(200, 25, 16, Fill.Solid)

      Draw.setColor(Color.Fill, 140, 90, 40, 255)
      Draw.circle(200, 70, 16, Fill.Outline)

      Draw.setColor(Color.Fill, 40, 90, 40, 255)
      Draw.tri(150, 150, 160, 160, 130, 160, Fill.Outline)

      Draw.setColor(Color.Stroke, 0, 255, 255, 255)
      Draw.tri(150, 180, 170, 170, 180, 180, Fill.Solid)

      Draw.image(_dog, 120, 120)
      
      Draw.bmpText(_font, 32, 50, "Good Dog!", 3.0)
      Draw.bmpText(_font, 32, 50, "Good Dog!", 2.0)
      Draw.bmpText(_font, 32, 50, "Good Dog!")

      Draw.bmpText(_fixedFont, 32, 34, "The Journey Of The")
      Draw.bmpText(_fixedFontBlue, 32, 42, "~Most Exceptional~")
      Draw.bmpText(_fixedFontBlue, 32, 100, "Type Y to start")


      Draw.sprite(_spr, 265, 300, 150, 1.0, 1.0, 0, 3, 3)
      Draw.sprite(_spr, 265, 295, 150, 0.5, 1.0, 0, 3, 3)

      Draw.sprite(_spr, 17, 250, 32)

      Draw.sprite(_spr, 17, 275, 0, 1.0, 1.0, 0, 2, 1)
      Draw.sprite(_spr, 17, 275, 8, 1.0, 1.0, 1, 2, 1)
      Draw.sprite(_spr, 17, 275, 16, 1.0, 1.0, 2, 2, 1)
      Draw.sprite(_spr, 17, 275, 24, 1.0, 1.0, 3, 2, 1)
      Draw.sprite(_spr, 17, 275, 32, 1.0, 1.0, 4, 2, 1)
      Draw.sprite(_spr, 17, 275, 48, 1.0, 1.0, 5, 2, 1)
      Draw.sprite(_spr, 17, 275, 64, 1.0, 1.0, 6, 2, 1)
      Draw.sprite(_spr, 17, 275, 80, 1.0, 1.0, 7, 2, 1)

      Draw.sprite(_spr, 17, 275, 96, 1.0, 2.0, 7, 2, 1)      

      if (_upActive) {
         Draw.rect(100, 0, 100, 100, Fill.Solid)
      }

      Draw.submit()
   }

   shutdown() {
      TileMap.free()
      Asset.clearAll()
   }
}