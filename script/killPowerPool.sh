sudo kill $(ps aux | grep '[p]ower_pool' | awk '{print $2}')
