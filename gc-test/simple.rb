#require 'GC'

#gc.disable
#Shallow graph
1000000.times do
  Hash.new
end
# GC.enable
# GC.start
