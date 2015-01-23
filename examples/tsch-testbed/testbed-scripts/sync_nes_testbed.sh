ssh $1 'testbed.py download'
rsync -Razv $1:jobs .
