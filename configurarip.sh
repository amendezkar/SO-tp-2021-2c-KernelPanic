perl -pi.bak -e "s/(?<=IP=).*/$1/g" kernel/cfg/*
perl -pi.bak -e "s/(?<=IP=).*/$2/g" memoria/cfg/*
perl -pi.bak -e "s/(?<=IP=).*/$3/g" swamp/cfg/*

perl -pi.bak -e "s/(?<=IP_SWAMP=).*/$3/g" memoria/cfg/*
perl -pi.bak -e "s/(?<=IP_MEMORIA=).*/$2/g" kernel/cfg/*
