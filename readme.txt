#Clone the source code
#Run the below commands in order 
docker run -ti -d --rm --name ran --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf
docker run -ti -d --rm --name ausf --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf
docker run -ti -d --rm --name amf --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf
docker run -ti -d --rm --name smf --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf
docker run -ti -d --rm --name upf --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf
docker run -ti -d --rm --name sink --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf

