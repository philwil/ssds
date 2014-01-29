/*
 * code for ssds_find_group
 * include in ssds_test.c and ssds_master.c
 * do not buld on its own
 */


int do_find_group(char *data, void *pcli, int group_act)
{
  int ret;
  int res;
  int i,j;
  int num;
  int len;
  char *dsp;
  char *tsp;
  int idx;

  char stmp[16];
  char ntmp[64];
  struct ssds_client *cli;
  struct ssds_client *cli_save;
  struct cli_group *gr;
  struct cli_group_entry *gr_ent;

  cli = pcli;
  ret = -1;
  switch(group_act) {

  case GROUP_ADD_ITEMS:
    gr = cli->curr_group;
    /* data contains the items */
    /* char num, item1, item2, item3 */
    dsp = (char *)data;
    tsp = stmp;
    while(*dsp && (*dsp != ',') && (*dsp != ':')) {
      *tsp++ = *dsp++;
    }
    *tsp = 0;
    dsp++;
    num = atoi(stmp);
    if(0)
      printf("set_group (%s) num entries now %d num %d (%s)dsp (%s)\n"
	     , gr->name, gr->num_entries, num, stmp,dsp);
    if(num<0) {
      num = -num;
      gr->num_entries=0;
    }
    if (gr->num_entries+num<gr->max_entries) {
      //gr->num_entries+=num;
      idx = gr->num_entries;
      while(num--) {
	gr_ent = &gr->entries[idx];
	tsp = gr_ent->name;
	len = 64-1;
	while(len-- && *dsp && (*dsp != ',') && (*dsp != ':')) {
	  *tsp++ = *dsp++;
	}
	*tsp = 0;
	//printf("make_group %d name (%s)dsp (%s)\n", idx, gr_ent->name, dsp);
	dsp++;
	idx++;
	gr->num_entries++;
      }
    }
    ret = gr->num_entries;
    break;

  case GROUP_SEE_ITEMS:
    gr = cli->curr_group;
    /* data contains the items */
    /* char num, item1, item2, item3 */
    num = gr->num_entries;
    if(0)printf("show_group num entries %d\n",num);
    ret = 0;
    dsp = (char *)data;
    ret+=sprintf(dsp+ret,"%d",num);

    for(idx=0; idx<num; idx++) {
      gr_ent = &gr->entries[idx];

      if(0)
	printf("show_group num entry %d name (%s)\n",
	     idx, gr_ent->name);

      ret+=sprintf(dsp+ret,",%s",gr_ent->name);
    }
    ret+=sprintf(dsp+ret,":");

    break;

  case GROUP_SET_ITEMS:
    /* set item values  from list of values*/
    ret = 0;
    dsp =  data;
    gr = cli->curr_group;
    num = gr->num_entries;
    ret += get_str_val(stmp, dsp+ret, ',', 32);
    sscanf(stmp,"%d",&num);
    //printf("set_items num %d dsp(%s)\n", num, dsp+ret);
    for(i=0;i<num;i++) {
      gr_ent = &gr->entries[i];
      ret += get_str_val(stmp, dsp+ret, ',', 32);
      if(0)
	printf("set_items %d name (%s) val (%s) ret %d dsp(%s)\n"
	     ,i,gr_ent->name,stmp,ret,dsp+ret);
      set_action_val_name(gr_ent->name, stmp, strlen(stmp));
    }
    break;

  case GROUP_GET_ITEMS:
    /* get item values  from list of values*/
    ret = 0;
    dsp =  data;
    gr = cli->curr_group;
    num=gr->num_entries;
    ret+=sprintf(dsp+ret,"%d",num);
    //printf("get_group (%s) num entries %d\n",gr->name,gr->num_entries);
    for(i=0;i<num;i++) {
      ret+=sprintf(dsp+ret,",");
      gr_ent = &gr->entries[i];
      ret+=get_action_val_name_gr(gr_ent->name, dsp+ret, 32);
      //printf("get_group %d name (%s) ret %d\n",i, gr_ent->name,ret);

    }
    ret+=sprintf(dsp+ret,":");
    break;

  case PRINT_GROUP:
    printf("num groups = %d\n",cli->num_groups);
    for(i=0;i<cli->num_groups;i++) {
      gr = &cli->groups[i];
      printf("groups %d name (%s) entries %d \n",
	     i, gr->name, gr->num_entries);
      for(j=0;j<gr->num_entries;j++) {
	gr_ent = &gr->entries[j];
	printf("   entry %d name (%s)\n",j, gr_ent->name);
      }
    }
    break;

  case GROUP_LEN_ITEMS_BIN:
    /* get item values  from list of values*/
    ret = 0;
    dsp =  data;
    gr = cli->curr_group;
    num=gr->num_entries;
    //ret+=sprintf(dsp+ret,"%d",num);
    if(0)printf("get_group (%s) num entries %d cli %p\n",
		gr->name,gr->num_entries, cli);
    for(i=0; i<num; i++) {
      //ret+=sprintf(dsp+ret,",",num);
      gr_ent = &gr->entries[i];
      res=get_action_len_name_gr(gr_ent->name, 32);
      if(res>0) ret+=res;
      if(0)
	printf("get_group_len name (%s) ret %d res %d\n", gr->name, ret, res);
    }
    //printf("get_group_len name (%s) len %d\n", gr->name,ret);
    break;

  case GROUP_GET_ITEMS_BIN:
    /* get bin item values  from list of values*/
    ret = 0;
    dsp =  data;
    gr = cli->curr_group;
    num=gr->num_entries;
    for(i=0;i<num;i++) {
      gr_ent = &gr->entries[i];
      res=get_action_val_name_bin(gr_ent->name, dsp+ret, 32);
      if(res > 0) ret+=res;
      if(0)
	printf("get_group_bin name %d (%s) ret %d res %d\n"
	       , i,gr_ent->name,ret,res);
    }
    break;

  case GROUP_SET_ITEMS_BIN:
    /* get bin item values  from list of values*/
    ret = 0;
    dsp =  data;
    gr = cli->curr_group;
    num=gr->num_entries;
    for(i=0;i<num;i++) {
      gr_ent = &gr->entries[i];
      res = set_action_val_name_bin(gr_ent->name, dsp+ret, 32);
      if(res > 0) ret += res;
    }
    break;


  case GROUP_FIND_NAME:
    cli_save = NULL;

    if (data[0] == '0') {
      cli_save = cli;
      cli=get_ssds_client(0);
      strncpy(ntmp, &data[1], 63);
      ntmp[63]=0;
      data = &ntmp[0];
    }

    if(cli) {
      for (i = 0; i < cli->num_groups; i++) {
	gr = &cli->groups[i];
	if(strcmp(gr->name,  data) == 0) {
	  ret = i;
	  if(cli_save) {
	    cli_save->curr_group = gr;
	  } else {
	    cli->curr_group = gr;
	  }
	  break;
	}
      }
      i = cli->num_groups;
      if((ret==-1) && (i < cli->max_groups)) {
	gr = &cli->groups[i];
	strncpy(gr->name, data, 63);
	gr->name[63] = 0;
	gr->num_entries = 0;
	gr->max_entries = NUM_GENTS;
	if(cli_save) {
	  cli_save->curr_group = gr;
	} else {
	  cli->curr_group = gr;
	}
        ret = i;
	cli->num_groups++;
      }
    }
    break;

  default:
    break;
  }
  return ret;
}
