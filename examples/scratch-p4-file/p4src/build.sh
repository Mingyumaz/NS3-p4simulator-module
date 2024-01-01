set -x

cd ./RED_SP_CoDel_1
p4c --target bmv2 --arch v1model codel1.p4 
p4c --target bmv2 --arch v1model codel2.p4
cd ./../RED_SP_CoDel_2
p4c --target bmv2 --arch v1model codel1.p4 
p4c --target bmv2 --arch v1model codel2.p4 
# cd ./../RED_SP_CoDel_3
# p4c --target bmv2 --arch v1model codel1.p4 
# p4c --target bmv2 --arch v1model codel2.p4 
# cd ./../RED_SP_CoDel_4
# p4c --target bmv2 --arch v1model codel1.p4
# p4c --target bmv2 --arch v1model codel2.p4
# cd ./../RED_SP_CoDel_5
# p4c --target bmv2 --arch v1model codel1.p4
# p4c --target bmv2 --arch v1model codel2.p4
cd ./..

python3 pre_processing_json.py ./RED_SP_CoDel_1/codel1.json 
python3 pre_processing_json.py ./RED_SP_CoDel_1/codel2.json 
python3 pre_processing_json.py ./RED_SP_CoDel_2/codel1.json 
python3 pre_processing_json.py ./RED_SP_CoDel_2/codel2.json 
# python3 pre_processing_json.py ./RED_SP_CoDel_3/codel1.json 
# python3 pre_processing_json.py ./RED_SP_CoDel_3/codel2.json 
# python3 pre_processing_json.py ./RED_SP_CoDel_4/codel1.json 
# python3 pre_processing_json.py ./RED_SP_CoDel_4/codel2.json 
# python3 pre_processing_json.py ./RED_SP_CoDel_5/codel1.json 
# python3 pre_processing_json.py ./RED_SP_CoDel_5/codel2.json 
set +x