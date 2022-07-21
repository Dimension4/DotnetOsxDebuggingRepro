require 'sketchup.rb'
require 'extensions.rb'

ext = SketchupExtension.new "MyPlugin", "MyPlugin/loader"
Sketchup.register_extension ext, true
