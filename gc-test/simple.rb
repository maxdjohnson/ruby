#require 'GC'

#gc.disable
#Shallow graph
100000.times do
  Object.new
end
# GC.enable
# GC.start
