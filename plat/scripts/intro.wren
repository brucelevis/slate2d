import "timer" for Timer
import "engine" for Draw, Asset, TMX, Trap
import "soundcontroller" for SoundController

class Intro {
  nextScene { _nextScene }
  nextScene=(params) { _nextScene = params }

  construct new(mapName) {
    var tmapId = Asset.create(Asset.TMX, mapName, mapName)
    TMX.setCurrent(tmapId)
    Asset.load(tmapId)

    var mapProps = TMX.getMapProperties()

    _nextScene = null

    _title = mapProps["properties"]["title"]

    _font = Asset.create(Asset.BitmapFont, "font", "gfx/good_neighbors.png")
    Asset.bmpfntSet(_font, "!\"#$\%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", 0, -1, 7, 16)

    _fixedFont = Asset.create(Asset.BitmapFont, "fixedfont", "gfx/panicbomber.png")
    Asset.bmpfntSet(_fixedFont, " !\"#$\%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", 8, 0, 8, 8)

    _fixedFontBlue = Asset.create(Asset.BitmapFont, "fixedfontblue", "gfx/panicbomber_blue.png")
    Asset.bmpfntSet(_fixedFontBlue, " !\"#$\%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", 8, 0, 8, 8)

    _music = Asset.create(Asset.Mod, "intromusic", "music/intro.mod")

    Asset.loadAll()

    _titleLen = Asset.textWidth(_font, _title)

    SoundController.playMusic(_music)

    Timer.runLater(180, Fn.new {
      _nextScene = ["world", mapName]
    })
  }

  update(dt) {

  }

  draw(w, h) {
    Draw.clear(0, 0, 0, 255)
    Draw.resetTransform()
    Draw.scale(h / 180)

    Draw.bmpText(_fixedFont, 20, 50, "Now entering...")
    Draw.bmpText( _font, (320 - _titleLen) / 2, 85, _title)
    Draw.bmpText(_fixedFontBlue, 220, 120, "Good Luck!")
  }

  shutdown() {

  }
}