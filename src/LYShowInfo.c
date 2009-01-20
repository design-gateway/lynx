/* $LynxId: LYShowInfo.c,v 1.69 2009/01/19 23:42:23 tom Exp $ */
#include <HTUtils.h>
#include <HTFile.h>
#include <HTParse.h>
#include <HTAlert.h>
#include <HTTP.h>
#include <LYCurses.h>
#include <LYUtils.h>
#include <LYStructs.h>
#include <LYGlobalDefs.h>
#include <LYShowInfo.h>
#include <LYCharUtils.h>
#include <GridText.h>
#include <LYReadCFG.h>
#include <LYCharSets.h>
#include <LYStrings.h>

#include <LYLeaks.h>

#ifdef DIRED_SUPPORT
#include <HTAAProt.h>
#include <time.h>
#include <LYLocal.h>
#endif /* DIRED_SUPPORT */

#define ADVANCED_INFO 1		/* to get more info in advanced mode */

#define BEGIN_DL(text) fprintf(fp0, "<h2>%s</h2>\n<dl compact>", LYEntifyTitle(&buffer, text))
#define END_DL()       fprintf(fp0, "\n</dl>\n")

#define ADD_SS(label,value)       dt_String(fp0, label, value)
#define ADD_NN(label,value,units) dt_Number(fp0, label, value, units)

static int label_columns;

/*
 * LYNX_VERSION and LYNX_DATE are automatically generated by PRCS, the tool
 * which we use to archive versions of Lynx.  We use a convention for naming
 * the successive versions:
 *	{release}{status}{patch}
 * where
 *	{release} is the release that we are working on, e.g., 2.8.4
 *	{status} is one of "dev", "pre" or "rel", and
 *	{patch} is a number assigned by PRCS.
 */
BOOL LYVersionIsRelease(void)
{
    return (BOOL) (strstr(LYNX_VERSION, "rel") != 0);
}

const char *LYVersionStatus(void)
{
    if (LYVersionIsRelease())
	return REL_VERSION;
    else if (strstr(LYNX_VERSION, "pre") != 0)
	return PRE_VERSION;
    return DEV_VERSION;
}

const char *LYVersionDate(void)
{
    static char temp[LYNX_DATE_LEN + 1];

    LYstrncpy(temp, &LYNX_DATE[LYNX_DATE_OFF], LYNX_DATE_LEN);
    return temp;
}

static void dt_String(FILE *fp,
		      const char *label,
		      const char *value)
{
    int have;
    int need;
    char *the_label = 0;
    char *the_value = 0;

    StrAllocCopy(the_label, label);
    StrAllocCopy(the_value, value);

    have = (int) strlen(the_label);
    need = LYstrExtent(the_label, have, label_columns);

    LYEntify(&the_label, TRUE);
    LYEntify(&the_value, TRUE);

    fprintf(fp, "<dt>");
    while (need++ < label_columns)
	fprintf(fp, "&nbsp;");
    fprintf(fp, "<em>%s</em> %s\n", the_label, the_value);

    FREE(the_label);
    FREE(the_value);
}

static void dt_Number(FILE *fp0,
		      const char *label,
		      long number,
		      const char *units)
{
    char *value = NULL;
    char *buffer = NULL;

    HTSprintf(&value, "%ld %s", number, LYEntifyTitle(&buffer, units));
    ADD_SS(label, value);
    FREE(value);
    FREE(buffer);
}

/*
 * LYShowInfo prints a page of info about the current file and the link that
 * the cursor is on.
 */
int LYShowInfo(DocInfo *doc,
	       DocInfo *newdoc,
	       char *owner_address)
{
    static char tempfile[LY_MAXPATH] = "\0";

    int url_type;
    FILE *fp0;
    char *Title = NULL;
    const char *cp;
    char *temp = 0;
    char *buffer = 0;

#ifdef ADVANCED_INFO
    BOOLEAN LYInfoAdvanced = (BOOL) (user_mode == ADVANCED_MODE);
#endif
#ifdef DIRED_SUPPORT
    struct stat dir_info;
    const char *name;
#endif /* DIRED_SUPPORT */

    if (LYReuseTempfiles) {
	fp0 = LYOpenTempRewrite(tempfile, HTML_SUFFIX, "w");
    } else {
	LYRemoveTemp(tempfile);
	fp0 = LYOpenTemp(tempfile, HTML_SUFFIX, "w");
    }
    if (fp0 == NULL) {
	HTAlert(CANNOT_OPEN_TEMP);
	return (-1);
    }

    /*
     * Point the address pointer at this Url
     */
    LYLocalFileToURL(&newdoc->address, tempfile);

    if (nlinks > 0 && links[doc->link].lname != NULL &&
	(url_type = is_url(links[doc->link].lname)) != 0 &&
	(url_type == LYNXEXEC_URL_TYPE ||
	 url_type == LYNXPROG_URL_TYPE)) {
	char *last_slash = strrchr(links[doc->link].lname, '/');
	int next_to_last = (int) strlen(links[doc->link].lname) - 1;

	if ((last_slash - links[doc->link].lname) == next_to_last) {
	    links[doc->link].lname[next_to_last] = '\0';
	}
    }

    label_columns = 9;

    WriteInternalTitle(fp0, SHOWINFO_TITLE);

    fprintf(fp0, "<h1>%s %s (%s) (<a href=\"%s\">%s</a>)",
	    LYNX_NAME, LYNX_VERSION,
	    LYVersionDate(),
	    (LYVersionIsRelease()? LYNX_WWW_HOME : LYNX_WWW_DIST),
	    LYVersionStatus());

    fprintf(fp0, "</h1>\n");	/* don't forget to close <h1> */

#ifdef DIRED_SUPPORT
    if (lynx_edit_mode && nlinks > 0) {

	BEGIN_DL(gettext("Directory that you are currently viewing"));

	temp = HTfullURL_toFile(doc->address);
	ADD_SS(gettext("Name:"), temp);
	FREE(temp);

	ADD_SS(gettext("URL:"), doc->address);

	END_DL();

	temp = HTfullURL_toFile(links[doc->link].lname);

	if (lstat(temp, &dir_info) == -1) {
	    CTRACE((tfp, "lstat(%s) failed, errno=%d\n", temp, errno));
	    HTAlert(CURRENT_LINK_STATUS_FAILED);
	} else {
	    char modes[80];

	    label_columns = 16;
	    if (S_ISDIR(dir_info.st_mode)) {
		BEGIN_DL(gettext("Directory that you have currently selected"));
	    } else if (S_ISREG(dir_info.st_mode)) {
		BEGIN_DL(gettext("File that you have currently selected"));
#ifdef S_IFLNK
	    } else if (S_ISLNK(dir_info.st_mode)) {
		BEGIN_DL(gettext("Symbolic link that you have currently selected"));
#endif
	    } else {
		BEGIN_DL(gettext("Item that you have currently selected"));
	    }
	    ADD_SS(gettext("Full name:"), temp);
#ifdef S_IFLNK
	    if (S_ISLNK(dir_info.st_mode)) {
		char buf[MAX_LINE];
		int buf_size;

		if ((buf_size = readlink(temp, buf, sizeof(buf) - 1)) != -1) {
		    buf[buf_size] = '\0';
		} else {
		    sprintf(buf, "%.*s", (int) sizeof(buf) - 1,
			    gettext("Unable to follow link"));
		}
		ADD_SS(gettext("Points to file:"), buf);
	    }
#endif
	    name = HTAA_UidToName((int) dir_info.st_uid);
	    if (*name)
		ADD_SS(gettext("Name of owner:"), name);
	    name = HTAA_GidToName((int) dir_info.st_gid);
	    if (*name)
		ADD_SS(gettext("Group name:"), name);
	    if (S_ISREG(dir_info.st_mode)) {
		ADD_NN(gettext("File size:"),
		       (long) dir_info.st_size,
		       gettext("(bytes)"));
	    }
	    /*
	     * Include date and time information.
	     */
	    ADD_SS(gettext("Creation date:"),
		   ctime(&dir_info.st_ctime));

	    ADD_SS(gettext("Last modified:"),
		   ctime(&dir_info.st_mtime));

	    ADD_SS(gettext("Last accessed:"),
		   ctime(&dir_info.st_atime));

	    END_DL();

	    label_columns = 9;
	    BEGIN_DL(gettext("Access Permissions"));
	    modes[0] = '\0';
	    modes[1] = '\0';	/* In case there are no permissions */
	    modes[2] = '\0';
	    if ((dir_info.st_mode & S_IRUSR))
		strcat(modes, ", read");
	    if ((dir_info.st_mode & S_IWUSR))
		strcat(modes, ", write");
	    if ((dir_info.st_mode & S_IXUSR)) {
		if (S_ISDIR(dir_info.st_mode))
		    strcat(modes, ", search");
		else {
		    strcat(modes, ", execute");
		    if ((dir_info.st_mode & S_ISUID))
			strcat(modes, ", setuid");
		}
	    }
	    ADD_SS(gettext("Owner:"), &modes[2]);

	    modes[0] = '\0';
	    modes[1] = '\0';	/* In case there are no permissions */
	    modes[2] = '\0';
	    if ((dir_info.st_mode & S_IRGRP))
		strcat(modes, ", read");
	    if ((dir_info.st_mode & S_IWGRP))
		strcat(modes, ", write");
	    if ((dir_info.st_mode & S_IXGRP)) {
		if (S_ISDIR(dir_info.st_mode))
		    strcat(modes, ", search");
		else {
		    strcat(modes, ", execute");
		    if ((dir_info.st_mode & S_ISGID))
			strcat(modes, ", setgid");
		}
	    }
	    ADD_SS(gettext("Group:"), &modes[2]);

	    modes[0] = '\0';
	    modes[1] = '\0';	/* In case there are no permissions */
	    modes[2] = '\0';
	    if ((dir_info.st_mode & S_IROTH))
		strcat(modes, ", read");
	    if ((dir_info.st_mode & S_IWOTH))
		strcat(modes, ", write");
	    if ((dir_info.st_mode & S_IXOTH)) {
		if (S_ISDIR(dir_info.st_mode))
		    strcat(modes, ", search");
		else {
		    strcat(modes, ", execute");
#ifdef S_ISVTX
		    if ((dir_info.st_mode & S_ISVTX))
			strcat(modes, ", sticky");
#endif
		}
	    }
	    ADD_SS(gettext("World:"), &modes[2]);
	    END_DL();
	}
	FREE(temp);
    } else {
#endif /* DIRED_SUPPORT */

	BEGIN_DL(gettext("File that you are currently viewing"));

	LYformTitle(&Title, doc->title);
	HTSprintf(&temp, "%s%s",
		  LYEntifyTitle(&buffer, Title),
		  ((doc->isHEAD &&
		    !strstr(Title, " (HEAD)") &&
		    !strstr(Title, " - HEAD")) ? " (HEAD)" : ""));
	ADD_SS(gettext("Linkname:"), temp);
	FREE(temp);

	ADD_SS("URL:", doc->address);

	if (HTLoadedDocumentCharset()) {
	    ADD_SS(gettext("Charset:"),
		   HTLoadedDocumentCharset());
	} else {
	    LYUCcharset *p_in = HTAnchor_getUCInfoStage(HTMainAnchor,
							UCT_STAGE_PARSER);

	    if (!p_in || isEmpty(p_in->MIMEname) ||
		HTAnchor_getUCLYhndl(HTMainAnchor, UCT_STAGE_PARSER) < 0) {
		p_in = HTAnchor_getUCInfoStage(HTMainAnchor, UCT_STAGE_MIME);
	    }
	    if (p_in && non_empty(p_in->MIMEname) &&
		HTAnchor_getUCLYhndl(HTMainAnchor, UCT_STAGE_MIME) >= 0) {
		HTSprintf(&temp, "%s %s",
			  LYEntifyTitle(&buffer, p_in->MIMEname),
			  gettext("(assumed)"));
		ADD_SS(gettext("Charset:"), p_in->MIMEname);
		FREE(temp);
	    }
	}

	if ((cp = HText_getServer()) != NULL && *cp != '\0')
	    ADD_SS(gettext("Server:"), cp);

	if ((cp = HText_getDate()) != NULL && *cp != '\0')
	    ADD_SS(gettext("Date:"), cp);

	if ((cp = HText_getLastModified()) != NULL && *cp != '\0')
	    ADD_SS(gettext("Last Mod:"), cp);

#ifdef ADVANCED_INFO
	if (LYInfoAdvanced) {
	    if (HTMainAnchor && HTMainAnchor->expires) {
		ADD_SS(gettext("Expires:"), HTMainAnchor->expires);
	    }
	    if (HTMainAnchor && HTMainAnchor->cache_control) {
		ADD_SS(gettext("Cache-Control:"), HTMainAnchor->cache_control);
	    }
	    if (HTMainAnchor && HTMainAnchor->content_length > 0) {
		ADD_NN(gettext("Content-Length:"),
		       HTMainAnchor->content_length,
		       gettext("bytes"));
	    } else {
		ADD_NN(gettext("Length:"),
		       HText_getNumOfBytes(),
		       gettext("bytes"));
	    }
	    if (HTMainAnchor && HTMainAnchor->content_language) {
		ADD_SS(gettext("Language:"), HTMainAnchor->content_language);
	    }
	}
#endif /* ADVANCED_INFO */

	if (doc->post_data) {
	    fprintf(fp0, "<dt><em>%s</em> <xmp>%.*s</xmp>\n",
		    LYEntifyTitle(&buffer, gettext("Post Data:")),
		    BStrLen(doc->post_data),
		    BStrData(doc->post_data));
	    ADD_SS(gettext("Post Content Type:"), doc->post_content_type);
	}

	ADD_SS(gettext("Owner(s):"),
	       (owner_address
		? owner_address
		: NO_NOTHING));

	ADD_NN(gettext("size:"),
	       HText_getNumOfLines(),
	       gettext("lines"));

	StrAllocCopy(temp,
		     ((lynx_mode == FORMS_LYNX_MODE)
		      ? gettext("forms mode")
		      : (HTisDocumentSource()
			 ? gettext("source")
			 : gettext("normal"))));
	if (doc->safe)
	    StrAllocCat(temp, gettext(", safe"));
	if (doc->internal_link)
	    StrAllocCat(temp, gettext(", via internal link"));

#ifdef ADVANCED_INFO
	if (LYInfoAdvanced) {
	    if (HText_hasNoCacheSet(HTMainText))
		StrAllocCat(temp, gettext(", no-cache"));
	    if (HTAnchor_isISMAPScript((HTAnchor *) HTMainAnchor))
		StrAllocCat(temp, gettext(", ISMAP script"));
	    if (doc->bookmark)
		StrAllocCat(temp, gettext(", bookmark file"));
	}
#endif /* ADVANCED_INFO */

	ADD_SS(gettext("mode:"), temp);
	FREE(temp);

	END_DL();

	if (nlinks > 0) {
	    BEGIN_DL(gettext("Link that you currently have selected"));
	    ADD_SS(gettext("Linkname:"),
		   LYGetHiliteStr(doc->link, 0));
	    if (lynx_mode == FORMS_LYNX_MODE &&
		links[doc->link].type == WWW_FORM_LINK_TYPE) {
		if (links[doc->link].l_form->submit_method) {
		    int method = links[doc->link].l_form->submit_method;
		    char *enctype = links[doc->link].l_form->submit_enctype;

		    ADD_SS(gettext("Method:"),
			   ((method == URL_POST_METHOD) ? "POST" :
			    ((method == URL_MAIL_METHOD) ? "(email)" :
			     "GET")));
		    ADD_SS(gettext("Enctype:"),
			   (non_empty(enctype)
			    ? enctype
			    : "application/x-www-form-urlencoded"));
		}
		if (links[doc->link].l_form->submit_action) {
		    ADD_SS(gettext("Action:"),
			   links[doc->link].l_form->submit_action);
		}
		if (!(links[doc->link].l_form->submit_method &&
		      links[doc->link].l_form->submit_action)) {
		    fprintf(fp0, "<dt>&nbsp;%s\n",
			    LYEntifyTitle(&buffer, gettext("(Form field)")));
		}
	    } else {
		ADD_SS("URL:",
		       NonNull(links[doc->link].lname));
	    }
	    END_DL();

	} else {
	    fprintf(fp0, "<h2>%s</h2>",
		    LYEntifyTitle(&buffer,
				  gettext("No Links on the current page")));
	}

#ifdef EXP_HTTP_HEADERS
	if ((cp = HText_getHttpHeaders()) != 0) {
	    fprintf(fp0, "<h2>%s</h2>",
		    LYEntifyTitle(&buffer, gettext("Server Headers:")));
	    fprintf(fp0, "<pre>%s</pre>",
		    LYEntifyTitle(&buffer, cp));
	}
#endif

#ifdef DIRED_SUPPORT
    }
#endif /* DIRED_SUPPORT */
    EndInternalPage(fp0);

    LYrefresh();

    LYCloseTemp(tempfile);
    FREE(Title);
    FREE(buffer);

    return (0);
}
