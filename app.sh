#!/bin/bash -e
APP_NAME=xmrig-proxy
ACTION=$1
. "$NVM_DIR/nvm.sh"

start() {
    NODE_ENV=production pm2 start index.js --interpreter=`nvm which stable` --name ${APP_NAME} --log log/${APP_NAME}.log --output log/${APP_NAME}.out --error log/${APP_NAME}.err
}

stop() {
    pm2 stop ${APP_NAME}
}

restart() {
    pm2 restart ${APP_NAME}
}

update() {
    git pull
    stop
    start
}

case "${ACTION}" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        restart
        ;;
    update)
        update
        ;;    
    *)
        echo $"Usage: $0 {start|stop|restart|update}"
        exit 1
esac
exit 0