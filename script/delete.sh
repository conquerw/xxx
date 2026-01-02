#!/bin/bash

set -e

delete_third_party()
{
	echo "start delete_third_party ..."
	
	rm -fr third_party
}

delete_app_obj()
{
	echo "start delete_app_obj ..."
	
	rm -fr build
}

delete_third_party
delete_app_obj
